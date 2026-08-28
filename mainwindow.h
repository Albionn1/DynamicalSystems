// mainwindow.h
#pragma once

#include "systems.h"

#include <QMainWindow>
#include <QTimer>
#include <QVector>
#include <QPointF>
#include <QPainter>
#include <QToolBar>
#include <QAction>
#include <QResizeEvent>
#include <QLabel>
#include <QPixmap>
#include <QElapsedTimer>
#include <QSvgRenderer>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QTimer>
#include <QWidget>

#include <QThread>
#include <QProgressDialog>
#include <QPointer>
#include <QMetaObject>
#include <QElapsedTimer>

#include <vector>
#include <deque>
#include <functional>

    class InitialConditionsDialog;
class HelpDialog;

// Forward declaration of visualization widget
class VisualizationWidget;

struct Candidate
{
    QPointF point;
    float strength;
    float edgeStrength;
    float textureStrength;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void paintVisualization(QPainter* p, const QRect& rect);
    void updateVisualizationCenter(const QSize& size);
    void loadCustomImage();

    void generateCustomImagePoints();

    void generateCustomImagePointsWorker(
        const QImage& sourceImage,
        std::vector<QPointF>& outputPoints,
        std::vector<float>& outputStrengths,
        const std::function<void(int)>& progressCallback
        );

    void startCustomImageProcessing();

    void applyPointDensityFilter(std::vector<Candidate>& candidates);
    void applyAdaptiveSpatialDistribution(std::vector<Candidate>& candidates);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QImage customImage_;

    std::vector<QPointF> customImagePoints_;
    std::vector<float> customImagePointStrengths_;

    QThread* customImageProcessingThread_ = nullptr;

    bool customImageProcessing_ = false;

    quint64 customImageProcessingRequest_ = 0;

    // Current animated particle positions.
    std::vector<QPointF> customImageParticlePositions_;

    // Current particle velocities.
    std::vector<QPointF> customImageParticleVelocities_;

    bool customImageActive_ = false;

    int customImageWidth_ = 400;
    int customImageHeight_ = 400;

    // double customImageStrength_ = 2.0;

    double customImageAnimationTime_ = 0.0;

    QPixmap customImageBuffer_;
    std::size_t lastDrawnIndex_ = 0;
    QSize lastWidgetSize_;

    void resetCustomImageBuffer();

    // ---------------------------------------------------------
    // Custom Image drawing
    // ---------------------------------------------------------

    std::size_t customImageDrawIndex_ = 0;

    bool customImageDrawingFinished_ = false;

    double customImageDrawSpeed_ = 220.0; // bigger number draws more points per timer step, making points drawing faster

    // =========================================================
    // CUSTOM IMAGE DYNAMICAL SYSTEM
    // =========================================================

    // Strength pulling particles toward the image attractor.
    double customImageAttraction_ = 1.35;

    // Tangential/curl component.
    double customImageCurl_ = 0.42;

    // Nonlinear field strength.
    double customImageNonlinear_ = 0.22;

    // Time-dependent perturbation.
    double customImageChaos_ = 0.08;

    // Particle damping.
    double customImageDamping_ = 0.985;

    // Maximum normalized particle velocity.
    double customImageMaxVelocity_ = 0.045;

    // Distance at which the image attraction starts becoming dominant.
    double customImageInfluenceRadius_ = 0.12;

    // Trail for every image particle
    std::vector<std::vector<QPointF>> customImageTrails_;

private:

    /*=========================================================
      ENUMS
    =========================================================*/

    enum class DrawMode
    {
        Trail,
        Poincare,
        Both
    };

    enum class OverlayMode
    {
        None,
        PhaseSpace,
        Energy,
        Lyapunov,
        Info
    };

    struct Crossing
    {
        QPointF pos;
        bool upward;
        int age;

        Crossing(const QPointF& p, bool u)
            : pos(p), upward(u), age(0)
        {
        }
    };

    /*=========================================================
      UI CREATION
    =========================================================*/

    void createSidebar();
    void createSidebarButton(const QString& text, const QString& icon, QAction* action, QLayout* layout);
    void createVisualizationWidget();

    /*=========================================================
      SIMULATION
    =========================================================*/

    void step();
    void resetState();
    void setSystem(int id);
    void setInitialConditions();

    QPointF project(const Vec& x);

    void saveSimulationImage(const QString& filename);

    /*=========================================================
      OVERLAYS
    =========================================================*/

    void drawPhaseSpace(QPainter* p);
    void drawEnergyOverlay(QPainter* p);
    void drawLyapunovOverlay(QPainter* p);
    void drawInfoOverlay(QPainter* p);

    void updateEnergy();
    void initLyapunov();
    void updateLyapunov();

    /*=========================================================
      DASHBOARD GRAPHS
    =========================================================*/

    void drawMiniEnergyGraph(QPainter& p,
                             const QRectF& rect);

    void drawMiniLyapunovGraph(QPainter& p,
                               const QRectF& rect);

    void drawEquationPanel(QPainter& p,
                           const QRectF& rect);

    /*=========================================================
      SIMULATION CORE
    =========================================================*/

    QTimer timer_;

    ODE system_;

    Vec state_;
    Vec state2_;

    int dims_ = 3;

    double dt_ = 0.01;

    int substeps_ = 10;

    bool simulationActive_ = false;
    bool simulationStarted_ = false;

    /*=========================================================
      TRAIL
    =========================================================*/

    std::deque<QPointF> trail_;

    int maxTrail_ = 5000;

    DrawMode drawMode_ = DrawMode::Trail;

    /*=========================================================
      POINCARE
    =========================================================*/

    std::deque<Crossing> poincarePoints_;

    bool poincareEnabled_ = false;

    bool poincareBothDirections_ = false;

    double poincarePlane_ = 25.0;

    /*=========================================================
      ENERGY
    =========================================================*/

    std::deque<double> energyHistory_;

    const int energyHistoryMax_ = 1000;

    /*=========================================================
      LYAPUNOV
    =========================================================*/

    std::deque<double> lyapunovDist_;

    const int lyapunovHistoryMax_ = 1000;

    bool lyapunovInitialized_ = false;

    double largestLyapunovEstimate_ = 0.0;

    /*=========================================================
      DOUBLE PENDULUM PARAMETERS
    =========================================================*/

    double m1_ = 1.0;
    double m2_ = 1.0;

    double L1_ = 1.0;
    double L2_ = 1.0;

    double g_ = 9.81;

    /*=========================================================
      VISUAL SETTINGS
    =========================================================*/

    QPointF center_;

    double scale_ = 8.0;

    QString systemName_ = "Lorenz";

    int colorMode_ = 0;

    bool fadingEnabled_ = true;

    bool gridEnabled_ = true;

    OverlayMode overlayMode_ = OverlayMode::None;

    /*=========================================================
      FORMULA CACHE
    =========================================================*/

    QPixmap formulaPixmap_;

    bool formulaNeedsUpdate_ = true;

    /*=========================================================
      PERFORMANCE
    =========================================================*/

    QElapsedTimer fpsTimer_;

    int frameCounter_ = 0;

    double currentFPS_ = 0.0;

    long long totalSteps_ = 0;

    double simulationTime_ = 0.0;

    /*=========================================================
      SIDEBAR
    =========================================================*/

    QFrame* sidebar_ = nullptr;
    QVBoxLayout* sidebarLayout_ = nullptr;
    QLabel* companyLabel_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QPushButton* pauseButton_ = nullptr;
    QString pauseButtonIcon_;


    /*=========================================================
      VISUALIZATION WIDGET
    =========================================================*/

    class VisualizationWidget* visualizationWidget_ = nullptr;

    /*=========================================================
      ACTIONS
    =========================================================*/

    QAction* saveAction_ = nullptr;
    QAction* pauseAction_ = nullptr;
    QAction* resetAction_ = nullptr;
    QAction* helpAction_ = nullptr;
    QAction* bothDirectionsAction_ = nullptr;

    /*=========================================================
      LEFT SIDEBAR (Currently not used)
    =========================================================*/

    // QDockWidget* leftDock_ = nullptr;
    // QPushButton* lorenzButton_ = nullptr;
    // QPushButton* rosslerButton_ = nullptr;
    // QPushButton* vdpButton_ = nullptr;
    // QPushButton* pendulumButton_ = nullptr;
    // QDoubleSpinBox* ic1Spin_ = nullptr;
    // QDoubleSpinBox* ic2Spin_ = nullptr;
    // QDoubleSpinBox* ic3Spin_ = nullptr;
    // QDoubleSpinBox* ic4Spin_ = nullptr;
    // QPushButton* resetICButton_ = nullptr;
    // QCheckBox* keyboardHelpCheck_ = nullptr;

    /*=========================================================
      RIGHT SIDEBAR (Currently not used)
    =========================================================*/

    // QDockWidget* rightDock_ = nullptr;
    // QComboBox* colorModeCombo_ = nullptr;
    // QSpinBox* trailLengthSpin_ = nullptr;
    // QCheckBox* gridCheck_ = nullptr;
    // QCheckBox* poincareCheck_ = nullptr;
    // QDoubleSpinBox* sectionPlaneSpin_ = nullptr;
    // QPushButton* overlayNoneButton_ = nullptr;
    // QPushButton* overlayPhaseButton_ = nullptr;
    // QPushButton* overlayEnergyButton_ = nullptr;
    // QPushButton* overlayLyapunovButton_ = nullptr;
    // QPushButton* overlayInfoButton_ = nullptr;

    /*=========================================================
      BOTTOM DASHBOARD (Currently not used)
    =========================================================*/

    // QWidget* bottomDashboard_ = nullptr;
    // QLabel* energyValueLabel_ = nullptr;
    // QLabel* lyapunovValueLabel_ = nullptr;
    // QLabel* equationTitleLabel_ = nullptr;

    /*=========================================================
      STATUS BAR (Currently not used)
    =========================================================*/

    // QLabel* statusSystemLabel_ = nullptr;
    // QLabel* statusMethodLabel_ = nullptr;
    // QLabel* statusDtLabel_ = nullptr;
    // QLabel* statusTimeLabel_ = nullptr;
    // QLabel* statusRunningLabel_ = nullptr;

};
