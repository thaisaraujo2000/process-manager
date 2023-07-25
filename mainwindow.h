#ifndef MAINWINDOW_H  // A diretiva de pré-processador #ifndef verifica se MAINWINDOW_H já foi definido. Se já foi definido, todo o código entre #ifndef e #endif é ignorado.
#define MAINWINDOW_H  // A diretiva #define define MAINWINDOW_H. Esta é uma técnica comum para evitar conflitos de inclusão múltipla.

#include <QMainWindow> // Inclui a classe QMainWindow do framework Qt que fornece a janela principal da aplicação.
#include <QTableWidgetItem> // Inclui a classe QTableWidgetItem do framework Qt que fornece um item para a QTableWidget.
#include <QThread> // Inclui a classe QThread do framework Qt que fornece uma maneira de gerenciar threads.
#include <QLabel> // Inclui a classe QLabel do framework Qt que fornece um widget de texto ou imagem.

// Define a estrutura ProcessStatus que contém informações sobre o status do processo.
struct ProcessStatus {
    QString ppid; // Armazena o ID do processo pai.
    QString status; // Armazena o status do processo.
};

namespace Ui {
class MainWindow; // Forward declaration da classe MainWindow dentro do namespace Ui.
}

class MainWindow : public QMainWindow  // Define a classe MainWindow que herda de QMainWindow.
{
    Q_OBJECT  // Macro do Qt que permite a classe usar sinais e slots e recursos de introspecção de tempo de execução.

public:
    explicit MainWindow(QWidget *parent = nullptr); // Construtor que aceita um ponteiro para um widget pai.
    ~MainWindow(); // Destrutor.

private slots:  // Define os slots privados.
    void updateProcesses(const QString& filter = ""); // Atualiza a lista de processos com base em um filtro.
    void killProcessSlot(); // Mata um processo.
    void updateProcessesWrapper(); // Chama updateProcesses em um wrapper.
    void pauseProcessSlot(); // Pausa um processo.
    void continueProcessSlot(); // Continua um processo.
    void applyCpuAffinitySlot(); // Aplica a afinidade da CPU a um processo.
    void showCpuAffinitySlot(); // Mostra a afinidade da CPU de um processo.
    void changeProcessPrioritySlot(); // Altera a prioridade de um processo.

private:
    bool killProcess(int pid); // Mata um processo com um determinado PID.
    bool pauseProcess(int pid); // Pausa um processo com um determinado PID.
    bool continueProcess(int pid); // Continua um processo com um determinado PID.
    bool setProcessCpuAffinity(int pid, int cpu); // Define a afinidade da CPU de um processo com um determinado PID para uma determinada CPU.
    QString getProcessCpuAffinity(int pid); // Obtém a afinidade da CPU de um processo com um determinado PID.
    Ui::MainWindow *ui;  // Ponteiro para a interface do usuário da MainWindow.

    // Definição das constantes
    static const int COLUMN_PID = 0; // Coluna para o PID.
    static const int COLUMN_NAME = 1; // Coluna para o nome do processo.
    static const int COLUMN_PPID = 2; // Coluna para o PPID.
    static const int COLUMN_CPU = 3; // Coluna para a CPU.
    static const int COLUMN_PRIORITY = 4; // Coluna para a prioridade.
    static const int COLUMN_STATE = 5; // Coluna para o status.
    static const int COLUMN_PRI = 6;
};

#endif // MAINWINDOW_H  // Encerra o #ifndef.
