#pragma once

#include "systems.h"

#include <QMainWindow>
#include <QTimer>
#include <QVector>
#include <QPointF>
#include <QPushButton>
#include <QToolBar>
#include <QAction>
#include <deque>
#include <QPixmap>
#include <QResizeEvent>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    enum class OverlayMode
    {
        None,
        PhaseSpace,
        Energy,
        Lyapunov,
        Info
    };
private:
    struct Crossing {
        QPointF pos;
        bool upward;
        int age;

        Crossing(const QPointF& p, bool u) : pos(p), upward(u), age(0) {}
    };

    OverlayMode overlayMode_ = OverlayMode::None;

    std::deque<double> energyHistory_;
    const int energyHistoryMax_ = 1000;

    Vec state2_;
    std::deque<double> lyapunovDist_;
    const int lyapunovHistoryMax_ = 1000;
    bool lyapunovInitialized_ = false;

    double m1_ = 1.0, m2_ = 1.0;
    double L1_ = 1.0, L2_ = 1.0;
    double g_ = 9.81;

    void drawPhaseSpace(QPainter& p);
    void drawEnergyOverlay(QPainter& p);
    void drawLyapunovOverlay(QPainter& p);
    void drawInfoOverlay(QPainter& p);

    void updateEnergy();
    void initLyapunov();
    void updateLyapunov();

    std::deque<Crossing> poincarePoints_;


    // Simulation
    QTimer timer_;
    ODE system_;
    Vec state_;
    double dt_ = 0.01;
    int dims_ = 3;
    int substeps_ = 10;
    bool paused_ = false;
    bool simulationActive_ = false;
    bool simulationStarted_ = false;

    // Trail
    QVector<QPointF> trail_;
    int maxTrail_ = 5000; // adjustable by user

    // Poincare section
    bool poincareBothDirections_ = false; // false = upward only, true = both
    double poincarePlane_ = 0.0; // z value of the Poincare plane (autoestimated)
    QAction* bothDirectionsAction_ = nullptr;
    bool poincareEnabled_ = false;

    // Drawing modes
    enum class DrawMode { Trail, Poincare, Both };
    DrawMode drawMode_ = DrawMode::Trail;

    // View transform
    double scale_ = 8.0; // pixels per unit
    QPointF center_;

    // Display
    QString systemName_;
    int colorMode_ = 0;              // 0=time, 1=speed
    bool fadingEnabled_ = true;
    bool gridEnabled_ = true;

    // Formula SVG cache
    QPixmap formulaPixmap_;
    bool formulaNeedsUpdate_ = true;

    // Toolbar
    QToolBar* toolbar_ = nullptr;
    QAction* saveAction_ = nullptr;
    QAction* pauseAction_ = nullptr;
    QAction* resetAction_ = nullptr;
    QAction* helpAction_ = nullptr;

    // Helpers
    void step();
    QPointF project(const Vec& x);
    void setSystem(int id);
    void resetState();
    void saveSimulationImage(const QString& filename);
    void setInitialConditions();
};
