#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess> // Para iniciar subprocessos
#include <QByteArray> // Para manipular arrays de bytes
#include <QTextStream> // Para leitura/escrita de dados de texto
#include <QTimer> // Para eventos de temporizador
#include <QRegularExpression> // Para expressões regulares
#include <signal.h> // Para manipulação de sinais
#include <errno.h> // Para manipulação de erros
#include <sched.h> // Para agendamento de processos
#include <sys/syscall.h> // Para chamadas de sistema
#include <sys/types.h> // Para tipos de dados de sistema
#include <sys/resource.h> // Para manipulação de recursos do sistema

// Construtor da MainWindow. Inicializa a interface de usuário e conecta sinais e slots
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), // Inicialize a classe base QMainWindow com o parâmetro parent
    ui(new Ui::MainWindow) // Crie uma nova instância da interface de usuário da MainWindow
{
    ui->setupUi(this); // Configurar a interface de usuário para esta instância da MainWindow

    // Conecte o sinal de que o texto do QLineEdit filter mudou ao slot updateProcessesWrapper desta instância da MainWindow
    connect(ui->filter, &QLineEdit::textChanged, this, &MainWindow::updateProcessesWrapper);
    QTimer *timer = new QTimer(this); // Crie um novo QTimer que tem esta instância da MainWindow como parent
    // Conecte o sinal de timeout do timer ao slot updateProcessesWrapper desta instância da MainWindow
    connect(timer, &QTimer::timeout, this, &MainWindow::updateProcessesWrapper);
    timer->start(1000); // Comece o temporizador com um intervalo de 1000 milissegundos
    // Conecte o sinal de clique do botão killButton ao slot killProcessSlot desta instância da MainWindow
    connect(ui->killButton, &QPushButton::clicked, this, &MainWindow::killProcessSlot);
    // Conecte o sinal de clique do botão stopButton ao slot pauseProcessSlot desta instância da MainWindow
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::pauseProcessSlot);
    // Conecte o sinal de clique do botão continueButton ao slot continueProcessSlot desta instância da MainWindow
    connect(ui->continueButton, &QPushButton::clicked, this, &MainWindow::continueProcessSlot);

    // Conecte o sinal de clique do botão applyCpuAffinityButton ao slot applyCpuAffinitySlot desta instância da MainWindow
    connect(ui->applyCpuAffinityButton, &QPushButton::clicked, this, &MainWindow::applyCpuAffinitySlot);
    // Conecte o sinal de clique do botão showCpuAffinityButton ao slot showCpuAffinitySlot desta instância da MainWindow
    connect(ui->showCpuAffinityButton, &QPushButton::clicked, this, &MainWindow::showCpuAffinitySlot);

    // Conecte o sinal de clique do botão applyPriorityButton ao slot changeProcessPrioritySlot desta instância da MainWindow
    connect(ui->applyPriorityButton, &QPushButton::clicked, this, &MainWindow::changeProcessPrioritySlot);

    // Adicione CPUs ao ComboBox
    int numCPUs = QThread::idealThreadCount(); // Obtenha o número ideal de threads
    for (int i = 0; i < numCPUs; i++) { // Para cada número de 0 até numCPUs (não incluindo numCPUs)
        // Adicione o número da CPU como um item ao comboBox de CPU
        ui->cpuComboBox->addItem(QString::number(i));
    }
}

// Destrutor da MainWindow. Deleta a instância da interface de usuário.
MainWindow::~MainWindow()
{
    delete ui; // Delete a instância da interface de usuário
}

// Método que atualiza a lista de processos exibida na interface do usuário com base no filtro fornecido.
void MainWindow::updateProcesses(const QString& filter)
{
    ui->tableWidget->setRowCount(0); // Defina o número de linhas da tabela para 0

    QProcess process; // Crie uma nova instância do QProcess
    // Comece um novo processo com o comando "ps" e os argumentos "axo" e "pid,comm,ppid,%cpu,nice,state, pri"
    process.start("ps", QStringList() << "axo" << "pid,comm,ppid,%cpu,nice,state,priority");
    process.waitForFinished(); // Espere até que o processo termine
    QByteArray output = process.readAllStandardOutput(); // Leia toda a saída padrão do processo

    QTextStream stream(&output); // Crie um novo QTextStream que lê do QByteArray output
    QString line; // Declare uma QString line
    stream.readLine(); // Leia e descarte a primeira linha (cabeçalho)

    // Enquanto houver linhas para ler no stream
    while (!(line = stream.readLine()).isNull())
    {
        // Divida a linha em partes baseadas em espaços em branco
        QStringList parts = line.trimmed().split(QRegularExpression("\\s+"));
        // Se houver pelo menos 6 partes
        if (parts.size() >= 7)
        {
            // Atribua cada parte a uma variável correspondente
            QString pid = parts[0];
            QString processName = parts[1];
            QString ppid = parts[2];
            QString cpuUsage = parts[3];
            QString priority = parts[4];
            QString status = parts[5];
            QString pri = parts[6];

            // Se o filtro estiver vazio ou o nome do processo contiver o filtro
            if (filter.isEmpty() || processName.contains(filter, Qt::CaseInsensitive))
            {
                // Insira uma nova linha na tabela
                int row = ui->tableWidget->rowCount();
                ui->tableWidget->insertRow(row);
                // Insira cada informação de processo em uma célula na nova linha
                ui->tableWidget->setItem(row, COLUMN_PID, new QTableWidgetItem(pid));
                ui->tableWidget->setItem(row, COLUMN_NAME, new QTableWidgetItem(processName));
                ui->tableWidget->setItem(row, COLUMN_PPID, new QTableWidgetItem(ppid));
                ui->tableWidget->setItem(row, COLUMN_CPU, new QTableWidgetItem(cpuUsage));
                ui->tableWidget->setItem(row, COLUMN_PRIORITY, new QTableWidgetItem(priority));
                ui->tableWidget->setItem(row, COLUMN_STATE, new QTableWidgetItem(status));
                ui->tableWidget->setItem(row, COLUMN_PRI, new QTableWidgetItem(pri));
            }
        }
    }
}

// Método que tenta matar o processo cujo PID é fornecido
bool MainWindow::killProcess(pid_t pid)
{
    // Tente matar o processo. Se a matança falhar, imprima um erro e retorne false. Se tiver sucesso, retorne true.
    int result = kill(pid, SIGKILL);
    if(result == -1) {
        qDebug() << "Kill failed with error: " << strerror(errno);
    }
    return (result == 0);
}

// Slot que é chamado quando o botão para matar um processo é clicado
void MainWindow::killProcessSlot()
{
    bool ok;
    // Tente converter o texto no campo de texto pid para um inteiro
    int pid = ui->pid_text->text().toInt(&ok);

    // Se a conversão falhou ou o PID é menor ou igual a 0, imprima um erro e retorne
    if (!ok || pid <= 0) {
        qDebug() << "Invalid PID";
        return;
    }

    // Tente matar o processo
    bool success = killProcess(pid);

    // Se a matança teve sucesso, imprima uma mensagem de sucesso. Caso contrário, imprima uma mensagem de erro.
    if (success) {
        qDebug() << "Successfully killed process" << pid;
    } else {
        qDebug() << "Failed to kill process" << pid;
    }
}

// Método que chama o método updateProcesses com o texto do filtro
void MainWindow::updateProcessesWrapper()
{
    updateProcesses(ui->filter->text());
}

// Método que tenta pausar o processo cujo PID é fornecido
bool MainWindow::pauseProcess(int pid)
{
    // Tenta parar o processo. Se falhar, imprima um erro e retorne false. Se tiver sucesso, retorne true.
    int result = kill(pid, SIGSTOP);
    if (result != 0) {
        qDebug() << "Error pausing process" << pid << ":" << strerror(errno);
    }
    return (result == 0);
}

// Slot que é chamado quando o botão para pausar um processo é clicado
void MainWindow::pauseProcessSlot()
{
    bool ok;
    // Tenta converter o texto no campo de texto pid para um inteiro
    int pid = ui->pid_text->text().toInt(&ok);

    // Se a conversão falhou ou o PID é menor ou igual a 0, imprima um erro e retorne
    if (!ok || pid <= 0) {
        qDebug() << "Invalid PID";
        return;
    }

    // Tenta pausar o processo
    bool success = pauseProcess(pid);

    // Se o processo foi pausado com sucesso, imprima uma mensagem de sucesso e atualize a lista de processos. Caso contrário, imprima uma mensagem de erro.
    if (success) {
        qDebug() << "Successfully paused process" << pid;
        updateProcessesWrapper();
    } else {
        qDebug() << "Failed to pause process" << pid;
    }
}

// Método que tenta continuar o processo cujo PID é fornecido
bool MainWindow::continueProcess(int pid)
{
    // Tente retomar o processo. Se falhar, imprima um erro e retorne false. Se tiver sucesso, retorne true.
    int result = kill(pid, SIGCONT);
    if (result != 0) {
        qDebug() << "Error continuing proCOLUMN_STATEcess" << pid << ":" << strerror(errno);
    }
    return (result == 0);
}

// Slot que é chamado quando o botão para continuar um processo é clicado
void MainWindow::continueProcessSlot()
{
    bool ok;
    // Tenta converter o texto no campo de texto pid para um inteiro
    int pid = ui->pid_text->text().toInt(&ok);

    // Se a conversão falhou ou o PID é menor ou igual a 0, imprima um erro e retorne
    if (!ok || pid <= 0) {
        qDebug() << "Invalid PID";
        return;
    }

    // Tenta retomar o processo
    bool success = continueProcess(pid);

    // Se o processo foi retomado com sucesso, imprima uma mensagem de sucesso e atualize a lista de processos. Caso contrário, imprima uma mensagem de erro.
    if (success) {
        qDebug() << "Successfully continued process" << pid;
        updateProcessesWrapper();
    } else {
        qDebug() << "Failed to continue process" << pid;
    }
}

// Slot que é chamado quando o botão para aplicar a afinidade de CPU é clicado.
void MainWindow::applyCpuAffinitySlot()
{
    bool ok;
    int pid = ui->pid_text->text().toInt(&ok);

    if (!ok || pid <= 0) {
        qDebug() << "Invalid PID";
        return;
    }

    int cpu = ui->cpuComboBox->currentText().toInt(&ok);

    if (!ok || cpu < 0) {
        qDebug() << "Invalid CPU number";
        return;
    }

    QString cpuAffinityBefore = getProcessCpuAffinity(pid);
    qDebug() << "CPU affinity before change:" << cpuAffinityBefore;

    bool success = setProcessCpuAffinity(pid, cpu);

    if (success) {
        qDebug() << "Successfully set CPU affinity for process" << pid << "to CPU" << cpu;
        QString cpuAffinityAfter = getProcessCpuAffinity(pid);
        ui->cpu_atual->setText(cpuAffinityAfter);
        qDebug() << "CPU affinity after change:" << cpuAffinityAfter;
    } else {
        qDebug() << "Failed to set CPU affinity for process" << pid;
    }
}

// Método que retorna a afinidade da CPU do processo especificado como uma string
QString MainWindow::getProcessCpuAffinity(int pid)
{
    QProcess process;
    process.start("taskset", QStringList() << "-p" << QString::number(pid));
    process.waitForFinished();
    QString output = process.readAllStandardOutput();

    // Get the CPU affinity mask as a hexadecimal string
    QString maskString = output.split(":").last().simplified();
    bool ok;
    int mask = maskString.toInt(&ok, 16); // Convert to integer

    if (!ok) {
        qDebug() << "Error converting CPU affinity mask to integer";
        return "Unknown";
    }

    // Create a list of CPUs
    QStringList cpuList;
    for (int i = 0; i < 8 * sizeof(int); i++) { // Assume maximum 32 CPUs
        if (mask & (1 << i)) {
            cpuList.append(QString::number(i));
        }
    }

    return "CPU " + cpuList.join(",");
}

// Método que define a afinidade da CPU do processo especificado
bool MainWindow::setProcessCpuAffinity(int pid, int cpu)
{
    QProcess process;
    process.start("taskset", QStringList() << "-pc" << QString::number(cpu) << QString::number(pid));
    process.waitForFinished();

    // Exit code 0 indicates that the command ran successfully
    return process.exitCode() == 0;
}

//Slot é chamado para exibir a afinidade da CPU do processo especificado
void MainWindow::showCpuAffinitySlot()
{
    bool ok;
    int pid = ui->pid_text->text().toInt(&ok);

    if (!ok || pid <= 0) {
        qDebug() << "Invalid PID";
        return;
    }

    QString cpuAffinity = getProcessCpuAffinity(pid);
    ui->cpu_atual->setText(cpuAffinity);
}

// Slot chamado quando a prioridade do processo precisa ser alterada
void MainWindow::changeProcessPrioritySlot()
{
    bool ok;
    int pid = ui->pid_text->text().toInt(&ok);

    if (!ok || pid <= 0) {
        qDebug() << "Invalid PID";
        return;
    }

    int priority = ui->priorityLineEdit->text().toInt(&ok);

    // Cria um objeto QProcess para executar o comando shell
    QProcess process;

    // Define o comando e os argumentos para o processo
    process.start("renice", QStringList() << QString::number(priority) << "-p" << QString::number(pid));

    // Aguarda o processo terminar
    process.waitForFinished();

    // Verifica se a chamada a renice foi bem sucedida.
    if (process.exitCode() == 0) {
        qDebug() << "Successfully set process priority for process" << pid << "to" << priority;
    } else {
        qDebug() << "Failed to set process priority for process" << pid << "Exit code:" << process.exitCode();
    }
}

