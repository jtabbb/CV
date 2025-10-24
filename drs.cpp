#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include "DRS.h"

class DRSWindow : public QMainWindow {
    Q_OBJECT

public:
    DRSWindow(QWidget* parent = nullptr)
        : QMainWindow(parent), drs(nullptr), board(nullptr), running(false) {

        QWidget* central = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(central);

        QLabel* header = new QLabel("DRS4 Evaluation Board Control", this);
        header->setStyleSheet("font-size: 18px; font-weight: bold;");
        layout->addWidget(header);

        // Board info
        infoBox = new QTextEdit(this);
        infoBox->setReadOnly(true);
        layout->addWidget(infoBox);

        // Channel selection
        QGroupBox* paramBox = new QGroupBox("Acquisition Settings", this);
        QVBoxLayout* paramLayout = new QVBoxLayout(paramBox);

        freqSpin = new QDoubleSpinBox(this);
        freqSpin->setRange(0.1, 5.0);
        freqSpin->setValue(0.7);
        freqSpin->setSuffix(" GHz");
        paramLayout->addWidget(new QLabel("Sampling Frequency:"));
        paramLayout->addWidget(freqSpin);

        chanSpin = new QSpinBox(this);
        chanSpin->setRange(1, 4);
        paramLayout->addWidget(new QLabel("Number of Channels:"));
        paramLayout->addWidget(chanSpin);

        trigEdits.resize(4);
        for (int i = 0; i < 4; ++i) {
            trigEdits[i] = new QDoubleSpinBox(this);
            trigEdits[i]->setRange(-1.0, 1.0);
            trigEdits[i]->setSingleStep(0.01);
            trigEdits[i]->setValue(0.05);
            paramLayout->addWidget(new QLabel(QString("Trigger Level Ch%1 (V):").arg(i + 1)));
            paramLayout->addWidget(trigEdits[i]);
        }

        layout->addWidget(paramBox);

        // Control buttons
        QHBoxLayout* btnLayout = new QHBoxLayout();
        startBtn = new QPushButton("Start Acquisition", this);
        stopBtn = new QPushButton("Stop", this);
        btnLayout->addWidget(startBtn);
        btnLayout->addWidget(stopBtn);
        layout->addLayout(btnLayout);

        connect(startBtn, &QPushButton::clicked, this, &DRSWindow::startAcquisition);
        connect(stopBtn, &QPushButton::clicked, this, &DRSWindow::stopAcquisition);

        setCentralWidget(central);
        resize(500, 600);

        // Initialize DRS board
        initBoard();
    }

private slots:
    void startAcquisition() {
        if (!board) {
            QMessageBox::warning(this, "Error", "No DRS4 board connected!");
            return;
        }

        running = true;
        infoBox->append("Starting acquisition...");
        board->SetFrequency(freqSpin->value(), true);
        board->SetTranspMode(1);
        board->SetInputRange(0);

        int nCh = chanSpin->value();
        for (int i = 0; i < nCh; ++i)
            board->SetIndividualTriggerLevel(i, trigEdits[i]->value());

        acquisitionTimer = new QTimer(this);
        connect(acquisitionTimer, &QTimer::timeout, this, &DRSWindow::acquireOnce);
        acquisitionTimer->start(100); // 10 Hz update
    }

    void stopAcquisition() {
        running = false;
        if (acquisitionTimer)
            acquisitionTimer->stop();
        infoBox->append("Acquisition stopped.");
    }

    void acquireOnce() {
        if (!board || !running) return;

        board->StartDomino();
        while (board->IsBusy());

        // Here you'd read waveforms and update a plot widget
        infoBox->append("Event acquired...");
    }

private:
    void initBoard() {
        drs = new DRS();
        int nBoards = drs->GetNumberOfBoards();
        if (nBoards == 0) {
            infoBox->append("No DRS4 evaluation board found.");
            return;
        }

        board = drs->GetBoard(0);
        infoBox->append(QString("Found DRS4 board, serial #%1").arg(board->GetBoardSerialNumber()));
        board->Init();
    }

private:
    DRS* drs;
    DRSBoard* board;
    bool running;
    QTimer* acquisitionTimer;

    QTextEdit* infoBox;
    QDoubleSpinBox* freqSpin;
    QSpinBox* chanSpin;
    std::vector<QDoubleSpinBox*> trigEdits;
    QPushButton* startBtn, * stopBtn;
};

#include "main.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    DRSWindow window;
    window.show();
    return app.exec();
}
