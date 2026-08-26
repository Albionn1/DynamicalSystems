#include "mainwindow.h"
#include "helpdialog.h"
#include "initialconditionsdialog.h"

#include <QPainter>
#include <QKeyEvent>
#include <QFileDialog>
#include <QDateTime>
#include <QPixmap>
#include <QMessageBox>
#include <QSvgRenderer>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <vector>
#include <random>

// Visualization Widget class
class VisualizationWidget : public QWidget {
public:
    explicit VisualizationWidget(
        MainWindow* mainWindow,
        QWidget* parent = nullptr)
        : QWidget(parent),
        mainWindow_(mainWindow)
    {
        setAttribute(Qt::WA_OpaquePaintEvent);
    }

protected:

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);

        if (mainWindow_) {
            QPainter p(this);
            mainWindow_->paintVisualization(
                &p,
                this->rect()
                );
        }
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);

        if (mainWindow_) {
            mainWindow_->updateVisualizationCenter(
                size()
                );
        }
    }

private:
    MainWindow* mainWindow_;
};


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Dynamical Systems via ODEs");
    resize(1600, 900);

    center_ = QPointF(800, 450);

    // ---------------------------------------------------------
    // Initialize simulation
    // ---------------------------------------------------------
    setSystem(1);
    resetState();

    // ---------------------------------------------------------
    // Create actions FIRST
    // ---------------------------------------------------------
    saveAction_ = new QAction("Save Image", this);
    pauseAction_ = new QAction("Pause", this);
    resetAction_ = new QAction("Reset", this);
    bothDirectionsAction_ = new QAction("Both Directions", this);
    helpAction_ = new QAction("Keyboard Shortcuts", this);

    bothDirectionsAction_->setCheckable(true);
    bothDirectionsAction_->setChecked(false);

    // ---------------------------------------------------------
    // Action connections
    // ---------------------------------------------------------
    connect(bothDirectionsAction_, &QAction::toggled,
            this, [this](bool checked) {
                poincareBothDirections_ = checked;
            });

    connect(saveAction_, &QAction::triggered,
            this, [this] {
                QString defaultName =
                    QString("snapshot_%1.png")
                        .arg(QDateTime::currentDateTime()
                                 .toString("yyyyMMdd_hhmmss"));

                QString filename = QFileDialog::getSaveFileName(
                    this,
                    "Save Simulation Image",
                    defaultName,
                    "PNG Images (*.png);;JPEG Images (*.jpg)"
                    );

                if (!filename.isEmpty())
                    saveSimulationImage(filename);
            });

    connect(pauseAction_, &QAction::triggered,
            this, [this] {
                simulationActive_ = !simulationActive_;
                simulationStarted_ =
                    simulationStarted_ || simulationActive_;

                if (pauseButton_) {
                    pauseButton_->setText(
                        pauseButtonIcon_ + "    " +
                        (simulationActive_ ? "Pause" : "Resume")
                        );
                }

                if (visualizationWidget_)
                    visualizationWidget_->update();
            });

    connect(resetAction_, &QAction::triggered,
            this, [this] {
                resetState();

                if (visualizationWidget_)
                    visualizationWidget_->update();
            });

    connect(helpAction_, &QAction::triggered,
            this, [this] {
                HelpDialog dlg(this);
                dlg.exec();
            });

    // ---------------------------------------------------------
    // Create UI AFTER actions exist
    // ---------------------------------------------------------
    createSidebar();
    createVisualizationWidget();

    // ---------------------------------------------------------
    // Simulation timer
    // ---------------------------------------------------------
    connect(&timer_, &QTimer::timeout,
            this, [this] {

                if (simulationActive_) {

                    // ---------------------------------------------------------
                    // Custom Image drawing animation
                    // ---------------------------------------------------------

                    if (customImageActive_) {

                        if (!customImageDrawingFinished_) {

                            // Draw a few points every frame.
                            customImageDrawIndex_ +=
                                static_cast<std::size_t>(
                                    customImageDrawSpeed_
                                    );

                            if (customImageDrawIndex_ >=
                                customImagePoints_.size()) {

                                customImageDrawIndex_ =
                                    customImagePoints_.size();

                                customImageDrawingFinished_ = true;
                            }
                        }
                    }

                    // -------------------------------------------------
                    // Normal dynamical-system simulation
                    // -------------------------------------------------
                    for (int i = 0;
                         i < substeps_;
                         ++i) {

                        step();
                    }


                    // -------------------------------------------------
                    // Request repaint
                    // -------------------------------------------------
                    if (visualizationWidget_)
                        visualizationWidget_->update();
                }
            });

    timer_.start(16);
}

void MainWindow::createSidebar()
{
    // ---------------------------------------------------------
    // Central container
    // ---------------------------------------------------------
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---------------------------------------------------------
    // Sidebar
    // ---------------------------------------------------------
    sidebar_ = new QFrame(centralWidget);
    sidebar_->setFixedWidth(250);

    sidebar_->setStyleSheet(
        "QFrame#sidebar {"
        "    background-color: #202124;"
        "    border-right: 1px solid #34363a;"
        "}"
        );

    sidebar_->setObjectName("sidebar");

    // ---------------------------------------------------------
    // Sidebar layout
    // ---------------------------------------------------------
    sidebarLayout_ = new QVBoxLayout(sidebar_);

    sidebarLayout_->setContentsMargins(18, 24, 18, 20);
    sidebarLayout_->setSpacing(8);

    // ---------------------------------------------------------
    // Brand
    // ---------------------------------------------------------
    auto* brandLabel = new QLabel("AKSIOMA", sidebar_);

    brandLabel->setStyleSheet(
        "QLabel {"
        "    color: #00d4ff;"
        "    font-size: 22px;"
        "    font-weight: 700;"
        "    letter-spacing: 3px;"
        "}"
        );

    brandLabel->setAlignment(Qt::AlignCenter);

    sidebarLayout_->addWidget(brandLabel);

    // Small subtitle
    auto* brandSubtitle =
        new QLabel("DYNAMICAL SYSTEMS", sidebar_);

    brandSubtitle->setStyleSheet(
        "QLabel {"
        "    color: #777b82;"
        "    font-size: 9px;"
        "    font-weight: 600;"
        "    letter-spacing: 2px;"
        "}"
        );

    brandSubtitle->setAlignment(Qt::AlignCenter);

    sidebarLayout_->addWidget(brandSubtitle);

    sidebarLayout_->addSpacing(18);

    // ---------------------------------------------------------
    // Section: Simulation
    // ---------------------------------------------------------
    auto* simulationLabel =
        new QLabel("SIMULATION", sidebar_);

    simulationLabel->setStyleSheet(
        "QLabel {"
        "    color: #666a70;"
        "    font-size: 9px;"
        "    font-weight: 700;"
        "    letter-spacing: 1.5px;"
        "    padding-left: 4px;"
        "    padding-bottom: 4px;"
        "}"
        );

    sidebarLayout_->addWidget(simulationLabel);

    createSidebarButton(
        "Save Image",
        "▣",
        saveAction_,
        sidebarLayout_
        );

    createSidebarButton(
        "Pause",
        "Ⅱ",
        pauseAction_,
        sidebarLayout_
        );

    createSidebarButton(
        "Reset",
        "↻",
        resetAction_,
        sidebarLayout_
        );

    sidebarLayout_->addSpacing(18);

    auto* systemsLabel =
        new QLabel("SYSTEMS", sidebar_);

    systemsLabel->setStyleSheet(
        "QLabel {"
        "    color: #666a70;"
        "    font-size: 9px;"
        "    font-weight: 700;"
        "    letter-spacing: 1.5px;"
        "    padding-left: 4px;"
        "    padding-bottom: 4px;"
        "}"
        );

    sidebarLayout_->addWidget(systemsLabel);
    auto* customImageButton =
        new QPushButton(sidebar_);

    customImageButton->setText("▧    Custom Image");
    customImageButton->setMinimumHeight(44);
    customImageButton->setCursor(Qt::PointingHandCursor);

    customImageButton->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    color: #bfc2c7;"
        "    border: 1px solid transparent;"
        "    border-radius: 8px;"
        "    padding: 0px 12px;"
        "    font-size: 12px;"
        "    font-weight: 500;"
        "    text-align: left;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2b2e32;"
        "    color: #ffffff;"
        "    border: 1px solid #383b40;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #30343a;"
        "}"
        );

    connect(
        customImageButton,
        &QPushButton::clicked,
        this,
        &MainWindow::loadCustomImage
        );

    sidebarLayout_->addWidget(customImageButton);



    // ---------------------------------------------------------
    // Section: Analysis
    // ---------------------------------------------------------
    auto* analysisLabel =
        new QLabel("ANALYSIS", sidebar_);

    analysisLabel->setStyleSheet(
        "QLabel {"
        "    color: #666a70;"
        "    font-size: 9px;"
        "    font-weight: 700;"
        "    letter-spacing: 1.5px;"
        "    padding-left: 4px;"
        "    padding-bottom: 4px;"
        "}"
        );

    sidebarLayout_->addWidget(analysisLabel);

    createSidebarButton(
        "Both Directions",
        "↔",
        bothDirectionsAction_,
        sidebarLayout_
        );

    createSidebarButton(
        "Keyboard Shortcuts",
        "⌘",
        helpAction_,
        sidebarLayout_
        );

    sidebarLayout_->addStretch();

    // ---------------------------------------------------------
    // System information panel
    // ---------------------------------------------------------
    auto* infoFrame = new QFrame(sidebar_);

    infoFrame->setStyleSheet(
        "QFrame {"
        "    background-color: #292b2f;"
        "    border: 1px solid #36383d;"
        "    border-radius: 8px;"
        "}"
        );

    auto* infoLayout = new QVBoxLayout(infoFrame);

    infoLayout->setContentsMargins(12, 10, 12, 10);
    infoLayout->setSpacing(5);

    auto* infoTitle =
        new QLabel("QUICK CONTROLS", infoFrame);

    infoTitle->setStyleSheet(
        "QLabel {"
        "    color: #00bcd4;"
        "    font-size: 9px;"
        "    font-weight: 700;"
        "    letter-spacing: 1px;"
        "}"
        );

    infoLayout->addWidget(infoTitle);

    auto* systemInfoLabel =
        new QLabel(
            "1 – 4    Switch system\n"
            "R       Reset simulation\n"
            "Space   Pause / Resume\n"
            "[ / ]   Change timestep\n"
            "+ / −   Zoom",
            infoFrame
            );

    systemInfoLabel->setStyleSheet(
        "QLabel {"
        "    color: #92969d;"
        "    font-size: 10px;"
        "    line-height: 1.5;"
        "}"
        );

    systemInfoLabel->setWordWrap(true);

    infoLayout->addWidget(systemInfoLabel);

    sidebarLayout_->addWidget(infoFrame);

    sidebarLayout_->addSpacing(12);

    // ---------------------------------------------------------
    // Version
    // ---------------------------------------------------------
    auto* versionLabel =
        new QLabel("AKSIOMA  •  v1.0", sidebar_);

    versionLabel->setStyleSheet(
        "QLabel {"
        "    color: #4f5258;"
        "    font-size: 8px;"
        "    letter-spacing: 1px;"
        "}"
        );

    versionLabel->setAlignment(Qt::AlignCenter);

    sidebarLayout_->addWidget(versionLabel);

    // ---------------------------------------------------------
    // Add sidebar to main layout
    // ---------------------------------------------------------
    mainLayout->addWidget(sidebar_);

    setCentralWidget(centralWidget);
}

void MainWindow::createVisualizationWidget()
{
    QWidget* central = this->centralWidget();

    if (!central)
        return;

    auto* mainLayout =
        qobject_cast<QHBoxLayout*>(central->layout());

    if (!mainLayout)
        return;

    visualizationWidget_ =
        new VisualizationWidget(this, central);

    visualizationWidget_->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
        );

    mainLayout->addWidget(
        visualizationWidget_,
        1
        );

    // The visualization's coordinate system starts at (0,0),
    // so its center should be based on its own size.
    center_ = QPointF(
        visualizationWidget_->width() / 2.0,
        visualizationWidget_->height() / 2.0
        );
}


void MainWindow::createSidebarButton(
    const QString& text,
    const QString& icon,
    QAction* action,
    QLayout* layout)
{
    auto* button = new QPushButton(sidebar_);

    button->setText(icon + "    " + text);

    button->setMinimumHeight(44);
    button->setCursor(Qt::PointingHandCursor);

    button->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    color: #bfc2c7;"
        "    border: 1px solid transparent;"
        "    border-radius: 8px;"
        "    padding: 0px 12px;"
        "    font-size: 12px;"
        "    font-weight: 500;"
        "    text-align: left;"
        "}"

        "QPushButton:hover {"
        "    background-color: #2b2e32;"
        "    color: #ffffff;"
        "    border: 1px solid #383b40;"
        "}"

        "QPushButton:pressed {"
        "    background-color: #30343a;"
        "}"

        "QPushButton:checked {"
        "    background-color: rgba(0, 188, 212, 0.12);"
        "    color: #00d4ff;"
        "    border: 1px solid rgba(0, 188, 212, 0.35);"
        "}"

        "QPushButton:checked:hover {"
        "    background-color: rgba(0, 188, 212, 0.18);"
        "}"
        );

    // ---------------------------------------------------------
    // Connect button to QAction
    // ---------------------------------------------------------
    if (action->isCheckable()) {

        button->setCheckable(true);
        button->setChecked(action->isChecked());

        connect(
            button,
            &QPushButton::toggled,
            action,
            &QAction::setChecked
            );

        connect(
            action,
            &QAction::toggled,
            button,
            &QPushButton::setChecked
            );

    } else {

        connect(
            button,
            &QPushButton::clicked,
            action,
            &QAction::triggered
            );
    }

    // ---------------------------------------------------------
    // Pause button
    // ---------------------------------------------------------
    if (text == "Pause") {
        pauseButton_ = button;
        pauseButtonIcon_ = icon;
    }

    layout->addWidget(button);
}


void MainWindow::resetState()
{
    trail_.clear();

    poincarePoints_.clear();


    // ---------------------------------------------------------
    // Reset custom image animation
    // ---------------------------------------------------------

    if (customImageActive_) {

        customImageDrawIndex_ = 0;

        customImageDrawingFinished_ = false;

        customImageAnimationTime_ = 0.0;
    }


    // ---------------------------------------------------------
    // Normal dynamical systems
    // ---------------------------------------------------------

    if (dims_ == 3) {

        state_ = {
            0.0,
            1.0,
            20.0
        };


        Vec tmp = state_;


        double sumz = 0.0;


        const int warmSteps =
            1000;


        for (int i = 0;
             i < warmSteps;
             ++i) {

            rk4_step(
                system_,
                tmp,
                dt_
                );


            sumz += tmp[2];
        }


        poincarePlane_ =
            sumz /
            double(warmSteps);


    } else if (dims_ == 2) {

        state_ = {
            1.0,
            0.0
        };


    } else if (dims_ == 4) {

        state_ = {
            M_PI / 2.0,
            0.0,
            M_PI / 2.0 + 0.1,
            0.0
        };
    }
}

void MainWindow::step() {
    if (!simulationActive_) return;

    Vec prev = state_;
    rk4_step(system_, state_, dt_);

    for (double v : state_) {
        if (!std::isfinite(v)) {
            resetState();
            return;
        }
    }

    QPointF p = project(state_);
    if (std::isfinite(p.x()) && std::isfinite(p.y())) {
        trail_.push_back(p);
        if (trail_.size() > maxTrail_) trail_.pop_front();
    }

    // Poincare section (only for 3D systems)
    if (dims_ == 3 && (drawMode_ == DrawMode::Poincare || drawMode_ == DrawMode::Both)) {
        for (auto& c : poincarePoints_) {
            c.age++;
        }
        while (!poincarePoints_.empty() && poincarePoints_.front().age > 300) {
            poincarePoints_.pop_front();
        }

        double prevZ = prev[2];
        double newZ  = state_[2];

        // Use configured/autoestimated Poincare plane
        double plane = poincarePlane_;
        bool upwardCross   = (prevZ < plane && newZ >= plane);
        bool downwardCross = (prevZ > plane && newZ <= plane);

        if (upwardCross || (poincareBothDirections_ && downwardCross)) {
            QPointF secPt(center_.x() + state_[0] * scale_,
                          center_.y() - state_[1] * scale_);

            // Store crossing with direction
            poincarePoints_.emplace_back(secPt, upwardCross);

            // Limit buffer size
            if (poincarePoints_.size() > 2000) {
                poincarePoints_.pop_front();
            }
        }
    }

    if (overlayMode_ == OverlayMode::Energy && dims_ == 4) {
        updateEnergy();
    }
    if (overlayMode_ == OverlayMode::Lyapunov) {
        if (!lyapunovInitialized_) initLyapunov();
        updateLyapunov(); }
}

QPointF MainWindow::project(const Vec& x)
{
    if (dims_ == 3) {
        return QPointF(
            center_.x() + x[0] * scale_,
            center_.y() - x[1] * scale_
            );
    }

    if (dims_ == 2) {
        return QPointF(
            center_.x() + x[0] * scale_,
            center_.y() - x[1] * scale_
            );
    }

    if (dims_ == 4) {

        // Double pendulum
        // Angles are measured from the downward vertical.

        const double th1 = x[0];
        const double th2 = x[2];

        // First pendulum bob
        const double x1 =
            L1_ * std::sin(th1);

        const double y1 =
            -L1_ * std::cos(th1);

        // Second pendulum bob
        const double x2 =
            x1 + L2_ * std::sin(th2);

        const double y2 =
            y1 - L2_ * std::cos(th2);

        return QPointF(
            center_.x() + x2 * scale_,
            center_.y() + y2 * scale_
            );
    }

    return center_;
}

void MainWindow::paintVisualization(
    QPainter* p,
    const QRect& rect)
{
    // =========================================================
    // BACKGROUND
    // =========================================================

    QLinearGradient bg(
        rect.topLeft(),
        rect.bottomLeft()
        );

    bg.setColorAt(
        0.0,
        QColor(30, 30, 35)
        );

    bg.setColorAt(
        0.5,
        QColor(25, 25, 30)
        );

    bg.setColorAt(
        1.0,
        QColor(20, 20, 25)
        );

    p->fillRect(
        rect,
        bg
        );


    // =========================================================
    // GENERAL ANTIALIASING
    // =========================================================

    p->setRenderHint(
        QPainter::Antialiasing,
        true
        );


    // =========================================================
    // GRID
    // =========================================================

    if (gridEnabled_) {

        p->setPen(
            QPen(
                QColor(50, 50, 55),
                1
                )
            );

        for (int x = 0;
             x < rect.width();
             x += 50) {

            p->drawLine(
                x,
                0,
                x,
                rect.height()
                );
        }

        for (int y = 0;
             y < rect.height();
             y += 50) {

            p->drawLine(
                0,
                y,
                rect.width(),
                y
                );
        }
    }


    // =========================================================
    // CUSTOM IMAGE
    // =========================================================

    if (customImageActive_ &&
        !customImage_.isNull()) {

        // =====================================================
        // COMPARISON MODE
        //
        // Before finished:
        //     One centered image.
        //
        // After finished:
        //     LEFT  = cyan point reconstruction
        //     RIGHT = original image at 50% opacity
        // =====================================================

        const bool comparisonMode =
            customImageDrawingFinished_;


        // =====================================================
        // POINT IMAGE RECTANGLE
        // =====================================================

        QRect targetRect;


        if (!comparisonMode) {

            // -------------------------------------------------
            // NORMAL GENERATION MODE
            // -------------------------------------------------

            QSize imageSize =
                customImage_.size();


            const double maxWidth =
                rect.width() * 0.70;


            const double maxHeight =
                rect.height() * 0.70;


            const double scaleFactor =
                std::min(
                    maxWidth /
                        static_cast<double>(
                            imageSize.width()
                            ),

                    maxHeight /
                        static_cast<double>(
                            imageSize.height()
                            )
                    );


            QSize targetSize(
                static_cast<int>(
                    imageSize.width() *
                    scaleFactor
                    ),

                static_cast<int>(
                    imageSize.height() *
                    scaleFactor
                    )
                );


            targetRect =
                QRect(
                    static_cast<int>(
                        center_.x() -
                        targetSize.width() / 2.0
                        ),

                    static_cast<int>(
                        center_.y() -
                        targetSize.height() / 2.0
                        ),

                    targetSize.width(),
                    targetSize.height()
                    );

        } else {

            // -------------------------------------------------
            // COMPARISON MODE
            //
            // Put point reconstruction on LEFT half.
            // -------------------------------------------------

            const int margin = 30;

            const int halfWidth =
                rect.width() / 2;


            const int availableWidth =
                halfWidth - margin * 2;


            const int availableHeight =
                rect.height() - margin * 2;


            const double imageAspect =
                static_cast<double>(
                    customImage_.width()
                    ) /
                static_cast<double>(
                    customImage_.height()
                    );


            int imageWidth =
                availableWidth;


            int imageHeight =
                static_cast<int>(
                    imageWidth /
                    imageAspect
                    );


            if (imageHeight > availableHeight) {

                imageHeight =
                    availableHeight;


                imageWidth =
                    static_cast<int>(
                        imageHeight *
                        imageAspect
                        );
            }


            targetRect =
                QRect(
                    margin +
                        (availableWidth -
                         imageWidth) / 2,

                    (rect.height() -
                     imageHeight) / 2,

                    imageWidth,
                    imageHeight
                    );
        }


        // =====================================================
        // ORIGINAL IMAGE
        // =====================================================

        if (!comparisonMode) {

            // -------------------------------------------------
            // During generation:
            // Keep faint original image underneath points.
            // -------------------------------------------------

            p->save();

            p->setOpacity(
                0.15
                );

            p->drawImage(
                targetRect,
                customImage_
                );

            p->restore();

        } else {

            // -------------------------------------------------
            // After generation:
            // Draw original image on RIGHT.
            // -------------------------------------------------

            const int margin = 30;

            const int halfWidth =
                rect.width() / 2;


            const int availableWidth =
                halfWidth - margin * 2;


            const int availableHeight =
                rect.height() - margin * 2;


            const double imageAspect =
                static_cast<double>(
                    customImage_.width()
                    ) /
                static_cast<double>(
                    customImage_.height()
                    );


            int imageWidth =
                availableWidth;


            int imageHeight =
                static_cast<int>(
                    imageWidth /
                    imageAspect
                    );


            if (imageHeight > availableHeight) {

                imageHeight =
                    availableHeight;


                imageWidth =
                    static_cast<int>(
                        imageHeight *
                        imageAspect
                        );
            }


            QRect originalRect(
                halfWidth +
                    (availableWidth -
                     imageWidth) / 2,

                (rect.height() -
                 imageHeight) / 2,

                imageWidth,
                imageHeight
                );


            // -------------------------------------------------
            // Original image at 50% opacity
            // -------------------------------------------------

            p->save();

            p->setOpacity(
                0.50
                );

            p->drawImage(
                originalRect,
                customImage_
                );

            p->restore();


            // -------------------------------------------------
            // Divider
            // -------------------------------------------------

            p->setPen(
                QPen(
                    QColor(70, 70, 80),
                    1
                    )
                );

            p->drawLine(
                halfWidth,
                20,
                halfWidth,
                rect.height() - 20
                );


            // -------------------------------------------------
            // Labels
            // -------------------------------------------------

            p->setRenderHint(
                QPainter::Antialiasing,
                true
                );

            p->setPen(
                QColor(
                    220,
                    220,
                    230
                    )
                );

            p->setFont(
                QFont(
                    "Monospace",
                    10,
                    QFont::Bold
                    )
                );


            p->drawText(
                targetRect.left(),
                targetRect.top() + 30,
                "POINT RECONSTRUCTION"
                );


            p->drawText(
                originalRect.left(),
                originalRect.top() + 30,
                "ORIGINAL IMAGE"
                );
        }


        // =====================================================
        // CUSTOM IMAGE POINT DRAWING
        // =========================================================

        // ---------------------------------------------------------
        // Make sure strength array matches point array.
        // ---------------------------------------------------------

        const std::size_t pointCount =
            std::min(
                customImagePoints_.size(),
                customImagePointStrengths_.size()
                );


        // ---------------------------------------------------------
        // Determine visible point count.
        // ---------------------------------------------------------

        const std::size_t count =
            std::min(
                customImageDrawIndex_,
                pointCount
                );


        // ---------------------------------------------------------
        // POINT RENDERING
        //
        // Everything remains point-based.
        // No original-image color is used.
        // ---------------------------------------------------------

        // =================================================
        // POINT RENDERING
        // =================================================

        if (count > 0) {

            p->setPen(Qt::NoPen);

            // -------------------------------------------------
            // PASS 1
            // Soft atmospheric glow
            //
            // Very subtle. This does NOT fill the image.
            // It only gives stronger points a little presence.
            // -------------------------------------------------

            for (std::size_t i = 0;
                 i < count;
                 ++i) {

                const float strength =
                    customImagePointStrengths_[i];

                if (strength < 0.45f)
                    continue;

                const QPointF& point =
                    customImagePoints_[i];

                QPointF screenPoint(
                    targetRect.left() +
                        point.x() *
                            targetRect.width(),

                    targetRect.top() +
                        point.y() *
                            targetRect.height()
                    );


                // Glow only becomes noticeable on stronger points.
                double glowStrength =
                    std::clamp(
                        (strength - 0.45) / 0.55,
                        0.0,
                        1.0
                        );


                double glowRadius =
                    2.2 +
                    glowStrength * 2.0;


                int glowAlpha =
                    static_cast<int>(
                        12.0 +
                        glowStrength * 28.0
                        );


                p->setBrush(
                    QColor(
                        0,
                        212,
                        255,
                        glowAlpha
                        )
                    );


                p->drawEllipse(
                    screenPoint,
                    glowRadius,
                    glowRadius
                    );
            }


            // -------------------------------------------------
            // PASS 2
            // Main points
            // -------------------------------------------------

            for (std::size_t i = 0;
                 i < count;
                 ++i) {

                const float strength =
                    customImagePointStrengths_[i];


                const QPointF& point =
                    customImagePoints_[i];


                QPointF screenPoint(
                    targetRect.left() +
                        point.x() *
                            targetRect.width(),

                    targetRect.top() +
                        point.y() *
                            targetRect.height()
                    );


                // -------------------------------------------------
                // Non-linear strength response
                //
                // Keeps weak points visible while giving strong
                // points more visual weight.
                // -------------------------------------------------

                double visualStrength =
                    std::pow(
                        std::clamp(
                            static_cast<double>(strength),
                            0.0,
                            1.0
                            ),
                        0.70
                        );


                // -------------------------------------------------
                // Alpha
                // -------------------------------------------------

                int alpha =
                    static_cast<int>(
                        45.0 +
                        visualStrength * 190.0
                        );


                alpha =
                    std::clamp(
                        alpha,
                        0,
                        255
                        );


                // -------------------------------------------------
                // Radius
                // -------------------------------------------------

                double radius =
                    0.65 +
                    visualStrength * 1.45;


                // -------------------------------------------------
                // Monochromatic cyan
                //
                // IMPORTANT:
                // No color is taken from the original image.
                // -------------------------------------------------

                int green =
                    static_cast<int>(
                        180.0 +
                        visualStrength * 35.0
                        );


                int blue =
                    static_cast<int>(
                        215.0 +
                        visualStrength * 40.0
                        );


                p->setBrush(
                    QColor(
                        0,
                        green,
                        blue,
                        alpha
                        )
                    );


                p->drawEllipse(
                    screenPoint,
                    radius,
                    radius
                    );
            }


            // -------------------------------------------------
            // PASS 3
            // Strong point cores
            //
            // These create the "sparkle" / definition.
            // -------------------------------------------------

            for (std::size_t i = 0;
                 i < count;
                 ++i) {

                const float strength =
                    customImagePointStrengths_[i];


                if (strength < 0.55f)
                    continue;


                const QPointF& point =
                    customImagePoints_[i];


                QPointF screenPoint(
                    targetRect.left() +
                        point.x() *
                            targetRect.width(),

                    targetRect.top() +
                        point.y() *
                            targetRect.height()
                    );


                double coreStrength =
                    std::clamp(
                        (static_cast<double>(strength) - 0.55) /
                            0.45,
                        0.0,
                        1.0
                        );


                double coreRadius =
                    0.45 +
                    coreStrength * 0.85;


                int coreAlpha =
                    static_cast<int>(
                        100.0 +
                        coreStrength * 130.0
                        );


                p->setBrush(
                    QColor(
                        30,
                        235,
                        255,
                        coreAlpha
                        )
                    );


                p->drawEllipse(
                    screenPoint,
                    coreRadius,
                    coreRadius
                    );
            }
        }

        // =====================================================
        // CUSTOM IMAGE HUD
        // =====================================================

        p->setRenderHint(
            QPainter::Antialiasing,
            true
            );


        p->setPen(
            QColor(
                220,
                220,
                230
                )
            );


        p->setFont(
            QFont(
                "Monospace",
                10
                )
            );


        const std::size_t visibleCount =
            std::min(
                customImageDrawIndex_,
                pointCount
                );


        p->drawText(
            10,
            20,

            QString(
                "System: Custom Image | Points: %1/%2"
                )
                .arg(
                    visibleCount
                    )
                .arg(
                    pointCount
                    )
            );


        // -----------------------------------------------------
        // Comparison status
        // -----------------------------------------------------

        if (comparisonMode) {

            p->setPen(
                QColor(
                    140,
                    145,
                    155
                    )
                );

            p->setFont(
                QFont(
                    "Monospace",
                    9
                    )
                );


            // p->drawText( //removed this because UI becomes too clustered
            //     10,
            //     40,
            //     "COMPARISON COMPLETE"
            //     );
        }


        // =====================================================
        // STOP HERE
        //
        // We do NOT draw the normal dynamical-system trail.
        // =====================================================

        return;
    }


    // =========================================================
    // NORMAL DYNAMICAL SYSTEM TRAIL
    // =========================================================

    if (
        (drawMode_ == DrawMode::Trail ||
         drawMode_ == DrawMode::Both)
        &&
        trail_.size() > 1
        ) {

        for (int i = 1;
             i < trail_.size();
             ++i) {

            double t =
                double(i) /
                trail_.size();


            QColor c;


            if (colorMode_ == 0) {

                c =
                    QColor::fromHsvF(
                        t,
                        1.0,
                        1.0,

                        fadingEnabled_
                            ?
                            (0.2 +
                             0.8 *
                                 (1.0 - t))
                            :
                            1.0
                        );

            } else {

                double dx =
                    trail_[i].x() -
                    trail_[i - 1].x();


                double dy =
                    trail_[i].y() -
                    trail_[i - 1].y();


                double speed =
                    std::sqrt(
                        dx * dx +
                        dy * dy
                        );


                double s =
                    std::min(
                        speed / 10.0,
                        1.0
                        );


                c =
                    QColor::fromHsvF(
                        0.3 +
                            0.7 * s,

                        1.0,
                        1.0,

                        fadingEnabled_
                            ?
                            (0.2 +
                             0.8 *
                                 (1.0 - t))
                            :
                            1.0
                        );
            }


            p->setPen(
                QPen(
                    c,
                    2
                    )
                );


            p->drawLine(
                trail_[i - 1],
                trail_[i]
                );
        }
    }


    // =========================================================
    // POINCARÉ SECTION
    // =========================================================

    if (
        drawMode_ == DrawMode::Poincare ||
        drawMode_ == DrawMode::Both
        ) {

        for (
            const auto& crossing :
            poincarePoints_
            ) {

            QColor col =
                crossing.upward
                    ?
                    QColor("#00aaff")
                    :
                    QColor("#ffffff");


            int alpha =
                static_cast<int>(
                    255 *
                    std::exp(
                        -crossing.age *
                        0.005
                        )
                    );


            col.setAlpha(
                alpha
                );


            p->setPen(
                Qt::NoPen
                );


            p->setBrush(
                col
                );


            p->drawEllipse(
                crossing.pos,
                3,
                3
                );


            QColor halo =
                col;


            halo.setAlpha(
                alpha / 3
                );


            p->setBrush(
                halo
                );


            p->drawEllipse(
                crossing.pos,
                6,
                6
                );
        }
    }


    // =========================================================
    // HUD
    // =========================================================

    p->setPen(
        QColor(
            220,
            220,
            230
            )
        );


    p->setFont(
        QFont(
            "Monospace",
            10
            )
        );


    int hudTop = 20;


    p->drawText(
        10,
        hudTop,

        QString(
            "System: %1 | dt=%2 | trail=%3/%4 | substeps=%5 | mode=%6"
            )
            .arg(systemName_)
            .arg(dt_)
            .arg(trail_.size())
            .arg(maxTrail_)
            .arg(substeps_)
            .arg(
                drawMode_ == DrawMode::Trail
                    ? "Trail"
                    :
                    drawMode_ == DrawMode::Poincare
                        ? "Poincaré"
                        :
                        "Both"
                )
        );


    hudTop += 20;


    // =========================================================
    // POINCARÉ HUD
    // =========================================================

    if (
        poincareEnabled_ &&
        (
            drawMode_ == DrawMode::Poincare ||
            drawMode_ == DrawMode::Both
            )
        ) {

        p->drawText(
            10,
            hudTop,

            QString(
                "Crossings: %1"
                )
                .arg(
                    poincareBothDirections_
                        ?
                        "Up + Down"
                        :
                        "Up only"
                    )
            );


        hudTop += 20;


        p->drawText(
            10,
            hudTop,

            QString(
                "Poincaré plane z = %1"
                )
                .arg(
                    poincarePlane_
                    )
            );


        hudTop += 20;


        p->drawText(
            10,
            hudTop,

            "Legend: White = Upward, Blue = Downward"
            );


        hudTop += 20;


    } else if (
        !poincareEnabled_ &&
        (
            drawMode_ == DrawMode::Poincare ||
            drawMode_ == DrawMode::Both
            )
        ) {

        p->drawText(
            10,
            hudTop,

            "Poincaré section not available for this system"
            );


        hudTop += 20;
    }


    // =========================================================
    // EDUCATIONAL OVERLAYS
    // =========================================================

    switch (overlayMode_) {

    case OverlayMode::None:
        break;

    case OverlayMode::PhaseSpace:
        drawPhaseSpace(p);
        break;

    case OverlayMode::Energy:
        if (dims_ == 4)
            drawEnergyOverlay(p);
        break;

    case OverlayMode::Lyapunov:
        drawLyapunovOverlay(p);
        break;

    case OverlayMode::Info:
        drawInfoOverlay(p);
        break;
    }


    // =========================================================
    // FORMULA SVG
    // =========================================================

    QString formulaPath;


    if (systemName_ == "Lorenz") {

        formulaPath =
            ":/images/images/lorenzEquationVector.svg";

    } else if (systemName_ == "Rössler") {

        formulaPath =
            ":/images/images/RosslerEquationVector.svg";

    } else if (systemName_ == "Van der Pol") {

        formulaPath =
            ":/images/images/VanDerPolEquationVector.svg";

    } else if (systemName_ == "Double Pendulum") {

        formulaPath =
            ":/images/images/DoublePendulumEquationVector.svg";
    }


    if (!formulaPath.isEmpty()) {

        QSvgRenderer renderer(
            formulaPath
            );


        QSizeF svgSize =
            renderer.defaultSize();


        if (svgSize.isEmpty())
            svgSize =
                QSizeF(
                    240,
                    140
                    );


        double scaleFactor =
            1.0;


        QSizeF targetSize;


        QRectF target;


        QRectF fullRect;


        QColor bgColor;


        QRectF renderRect;


        if (
            systemName_ ==
            "Double Pendulum"
            ) {

            double maxWidth =
                rect.width() *
                0.70;


            if (
                svgSize.width() >
                maxWidth
                ) {

                scaleFactor =
                    maxWidth /
                    svgSize.width();
            }


            targetSize =
                QSizeF(
                    svgSize.width() *
                        scaleFactor,

                    svgSize.height() *
                        scaleFactor
                    );


            target =
                QRectF(
                    rect.width() -
                        targetSize.width() -
                        30,

                    rect.height() -
                        targetSize.height() -
                        30,

                    targetSize.width(),
                    targetSize.height()
                    );


            fullRect =
                target.adjusted(
                    -10,
                    -10,
                    10,
                    10
                    );


            bgColor =
                QColor(
                    45,
                    45,
                    50,
                    90
                    );


            renderRect =
                QRectF(
                    10,
                    10,
                    targetSize.width(),
                    targetSize.height()
                    );

        } else {

            double maxWidth =
                rect.width() *
                0.15;


            if (
                svgSize.width() >
                maxWidth
                ) {

                scaleFactor =
                    maxWidth /
                    svgSize.width();
            }


            targetSize =
                QSizeF(
                    svgSize.width() *
                        scaleFactor,

                    svgSize.height() *
                        scaleFactor
                    );


            target =
                QRectF(
                    rect.width() -
                        targetSize.width() -
                        20,

                    rect.height() -
                        targetSize.height() -
                        20,

                    targetSize.width(),
                    targetSize.height()
                    );


            fullRect =
                target.adjusted(
                    -8,
                    -8,
                    8,
                    8
                    );


            bgColor =
                QColor(
                    45,
                    45,
                    50,
                    200
                    );


            renderRect =
                QRectF(
                    8,
                    8,
                    targetSize.width(),
                    targetSize.height()
                    );
        }


        if (formulaNeedsUpdate_) {

            formulaPixmap_ =
                QPixmap(
                    fullRect.size().toSize()
                    );


            formulaPixmap_.fill(
                Qt::transparent
                );


            QPainter pixPainter(
                &formulaPixmap_
                );


            pixPainter.fillRect(
                formulaPixmap_.rect(),
                bgColor
                );


            pixPainter.setPen(
                QPen(
                    QColor(
                        200,
                        200,
                        210
                        ),
                    1
                    )
                );


            pixPainter.drawRect(
                formulaPixmap_.rect()
                    .adjusted(
                        1,
                        1,
                        -1,
                        -1
                        )
                );


            renderer.render(
                &pixPainter,
                renderRect
                );


            formulaNeedsUpdate_ =
                false;
        }


        p->drawPixmap(
            fullRect.topLeft(),
            formulaPixmap_
            );
    }


    // =========================================================
    // START MESSAGE
    // =========================================================

    if (!simulationStarted_) {

        p->setFont(
            QFont(
                "Monospace",
                12,
                QFont::Bold
                )
            );


        p->setPen(
            Qt::yellow
            );


        p->drawText(
            rect,
            Qt::AlignCenter,
            "Press 1–4 to start a system"
            );


    } else if (!simulationActive_) {

        QRect box(
            rect.width() / 2 - 60,
            50,
            120,
            30
            );


        p->setBrush(
            Qt::red
            );


        p->setPen(
            Qt::NoPen
            );


        p->drawRect(
            box
            );


        p->setPen(
            Qt::white
            );


        p->setFont(
            QFont(
                "Monospace",
                10
                )
            );


        p->drawText(
            box,
            Qt::AlignCenter,
            "PAUSED"
            );
    }
}

void MainWindow::saveSimulationImage(const QString& filename)
{
    if (!visualizationWidget_)
        return;

    QPixmap pixmap =
        visualizationWidget_->grab();

    if (!pixmap.save(filename)) {
        QMessageBox::warning(
            this,
            "Save Error",
            "Could not save the simulation image."
            );
    }
}
void MainWindow::setSystem(int id) {

    customImageActive_ = false;

    customImagePoints_.clear();

    customImageDrawIndex_ = 0;

    customImageDrawingFinished_ = false;

    customImageAnimationTime_ = 0.0;

    customImage_ = QImage();

    switch (id) {
    case 1:
        system_ = lorenz();
        dims_ = 3;
        scale_ = 8.0;
        systemName_ = "Lorenz";
        poincareEnabled_ = true;
        break;
    case 2:
        system_ = rossler();
        dims_ = 3;
        scale_ = 30.0;
        systemName_ = "Rössler";
        poincareEnabled_ = true;
        break;
    case 3:
        system_ = van_der_pol(5.0);
        dims_ = 2;
        scale_ = 80.0;
        systemName_ = "Van der Pol";
        poincareEnabled_ = false;
        break;
    case 4:
        system_ = double_pendulum();
        dims_ = 4;
        scale_ = 180.0;
        systemName_ = "Double Pendulum";
        poincareEnabled_ = false;
        break;
    default:
        system_ = lorenz();
        dims_ = 3;
        scale_ = 8.0;
        systemName_ = "Lorenz";
        poincareEnabled_ = true;
        break;
    }

    // Reset overlays
    energyHistory_.clear();
    lyapunovDist_.clear();
    lyapunovInitialized_ = false;

    // Default physical params for double pendulum
    m1_ = 1.0;
    m2_ = 1.0;
    L1_ = 1.0;
    L2_ = 1.0;
    g_ = 9.81;

    formulaNeedsUpdate_ = true;

    resetState();

}

void MainWindow::drawPhaseSpace(QPainter* p) {
    QRectF inset(p->device()->width() - 300, 50, 250, 250);
    p->fillRect(inset, QColor(45, 45, 50));
    p->setPen(QPen(QColor(200, 200, 210), 1));
    p->drawRect(inset);

    p->setFont(QFont("Monospace", 9));
    p->drawText(inset.left() + 8, inset.top() + 18, "Phase space");

    if (trail_.size() < 2) return;

    // Compute bounding box of projected trail
    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    for (const QPointF& pt : trail_) {
        minX = std::min(minX, pt.x());
        maxX = std::max(maxX, pt.x());
        minY = std::min(minY, pt.y());
        maxY = std::max(maxY, pt.y());
    }

    double rangeX = maxX - minX;
    double rangeY = maxY - minY;
    if (rangeX < 1e-6) rangeX = 1.0;
    if (rangeY < 1e-6) rangeY = 1.0;

    // Scale to fit inset (leave margin)
    double sx = inset.width() / rangeX;
    double sy = inset.height() / rangeY;
    double s = std::min(sx, sy) * 0.9;

    p->setPen(QPen(QColor(120, 120, 130), 1, Qt::DashLine));
    p->drawLine(inset.left(), inset.center().y(), inset.right(), inset.center().y());
    p->drawLine(inset.center().x(), inset.top(), inset.center().x(), inset.bottom());

    // Axis labels
    p->setFont(QFont("Monospace", 8));
    if (dims_ == 4) {
        p->drawText(inset.right() - 20, inset.center().y() - 5, "θ1");
        p->drawText(inset.center().x() + 5, inset.top() + 15, "θ2");
    } else {
        p->drawText(inset.right() - 15, inset.center().y() - 5, "x");
        p->drawText(inset.center().x() + 5, inset.top() + 15, "y");
    }

    p->setPen(QPen(QColor("#ffaa00"), 1));
    for (int i = 1; i < trail_.size(); ++i) {
        QPointF a(inset.left() + (trail_[i-1].x() - minX) * s,
                  inset.bottom() - (trail_[i-1].y() - minY) * s);
        QPointF b(inset.left() + (trail_[i].x() - minX) * s,
                  inset.bottom() - (trail_[i].y() - minY) * s);
        p->drawLine(a, b);
    }
}

void MainWindow::updateEnergy() {
    // State: [theta1, omega1, theta2, omega2]
    double th1 = state_[0], w1 = state_[1];
    double th2 = state_[2], w2 = state_[3];

    // Positions
    double x1 = L1_ * std::sin(th1);
    double y1 = -L1_ * std::cos(th1);
    double x2 = x1 + L2_ * std::sin(th2);
    double y2 = y1 - L2_ * std::cos(th2);

    // Velocities
    double vx1 = L1_ * w1 * std::cos(th1);
    double vy1 = L1_ * w1 * std::sin(th1);
    double vx2 = vx1 + L2_ * w2 * std::cos(th2);
    double vy2 = vy1 + L2_ * w2 * std::sin(th2);

    double KE = 0.5 * m1_ * (vx1*vx1 + vy1*vy1) + 0.5 * m2_ * (vx2*vx2 + vy2*vy2);
    double PE = m1_ * g_ * (y1) + m2_ * g_ * (y2); // y is negative down; relative energy OK

    double E = KE + PE;
    energyHistory_.push_back(E);
    if (energyHistory_.size() > energyHistoryMax_) energyHistory_.pop_front();
}

void MainWindow::drawEnergyOverlay(QPainter* p) {
    QRectF inset(p->device()->width() - 300, 50, 250, 250);
    p->fillRect(inset, QColor(45, 45, 50));
    p->setPen(QPen(QColor(200, 200, 210), 1));
    p->drawRect(inset);
    p->setFont(QFont("Monospace", 9));
    p->drawText(inset.left() + 8, inset.top() + 18, "Total energy (KE+PE)");

    if (energyHistory_.size() < 2) return;

    // Normalize to fit
    double minE = *std::min_element(energyHistory_.begin(), energyHistory_.end());
    double maxE = *std::max_element(energyHistory_.begin(), energyHistory_.end());
    double range = std::max(1e-6, maxE - minE);

    p->setPen(QPen(QColor("#66ccff"), 2));
    for (int i = 1; i < energyHistory_.size(); ++i) {
        double t0 = double(i-1) / (energyHistory_.size()-1);
        double t1 = double(i)   / (energyHistory_.size()-1);
        QPointF a(inset.left() + t0 * inset.width(),
                  inset.bottom() - ((energyHistory_[i-1] - minE) / range) * inset.height());
        QPointF b(inset.left() + t1 * inset.width(),
                  inset.bottom() - ((energyHistory_[i]   - minE) / range) * inset.height());
        p->drawLine(a, b);
    }
}

void MainWindow::initLyapunov() {
    state2_ = state_;
    // Small perturbation on first coordinate
    if (!state2_.empty()) state2_[0] += 1e-6;
    lyapunovDist_.clear();
    lyapunovInitialized_ = true;
}

void MainWindow::updateLyapunov() {
    // Integrate second trajectory with same system and dt
    rk4_step(system_, state2_, dt_);

    // Distance in state space
    double d = 0.0;
    int n = std::min<int>(state_.size(), state2_.size());
    for (int i = 0; i < n; ++i) {
        double di = state_[i] - state2_[i];
        d += di * di;
    }
    d = std::sqrt(d);

    lyapunovDist_.push_back(d);
    if (lyapunovDist_.size() > lyapunovHistoryMax_) lyapunovDist_.pop_front();
}

void MainWindow::drawLyapunovOverlay(QPainter* p) {
    QRectF inset(p->device()->width() - 300, 50, 250, 250);
    p->fillRect(inset, QColor(45, 45, 50));
    p->setPen(QPen(QColor(200, 200, 210), 1));
    p->drawRect(inset);
    p->setFont(QFont("Monospace", 9));
    p->drawText(inset.left() + 8, inset.top() + 18, "Trajectory divergence (|Δstate|)");

    if (lyapunovDist_.size() < 2) return;

    double minD = *std::min_element(lyapunovDist_.begin(), lyapunovDist_.end());
    double maxD = *std::max_element(lyapunovDist_.begin(), lyapunovDist_.end());
    double range = std::max(1e-12, maxD - minD);

    p->setPen(QPen(QColor("#ff6688"), 2));
    for (int i = 1; i < lyapunovDist_.size(); ++i) {
        double t0 = double(i-1) / (lyapunovDist_.size()-1);
        double t1 = double(i)   / (lyapunovDist_.size()-1);
        QPointF a(inset.left() + t0 * inset.width(),
                  inset.bottom() - ((lyapunovDist_[i-1] - minD) / range) * inset.height());
        QPointF b(inset.left() + t1 * inset.width(),
                  inset.bottom() - ((lyapunovDist_[i]   - minD) / range) * inset.height());
        p->drawLine(a, b);
    }
}

void MainWindow::drawInfoOverlay(QPainter* p) {
    QRectF inset(p->device()->width() - 420, 50, 400, 100);
    p->fillRect(inset, QColor(45, 45, 50, 230));
    p->setPen(QPen(QColor(220, 220, 230), 1));
    p->drawRect(inset);

    p->setFont(QFont("Monospace", 10));
    QString title = QString("%1 — About").arg(systemName_);
    p->drawText(inset.left() + 10, inset.top() + 22, title);

    p->setFont(QFont("Monospace", 9));
    QString body;
    if (systemName_ == "Lorenz") {
        body = "Models atmospheric convection.\n"
               "Famous for deterministic chaos and the butterfly effect.\n"
               "Parameters (σ, ρ, β) shape the attractor.";
    } else if (systemName_ == "Rössler") {
        body = "Simple chaotic system with spiral dynamics.\n"
               "Shows a strange attractor with twisting and folding.\n"
               "Parameters (a, b, c) control spiral and damping.";
    } else if (systemName_ == "Van der Pol") {
        body = "Nonlinear oscillator with self-sustained oscillations.\n"
               "Used in circuits and biology.\n"
               "μ controls nonlinearity and relaxation behavior.";
    } else if (systemName_ == "Double Pendulum") {
        body = "Two coupled pendulums—classic chaotic motion.\n"
               "Energy exchanges between arms; sensitive to initial conditions.\n"
               "Angles (θ1, θ2) and their velocities define the state.";
    }
    // Draw multiline
    int y = inset.top() + 44;
    for (const QString& line : body.split('\n')) {
        p->drawText(inset.left() + 10, y, line);
        y += 18;
    }
}

void MainWindow::setInitialConditions() {
    InitialConditionsDialog dlg(systemName_, this);
    if (dlg.exec() == QDialog::Accepted) {
        std::vector<double> vals = dlg.values();

        // Ensure the provided vector matches the expected dimensionality
        if (vals.size() != static_cast<size_t>(dims_)) {
            // If too short, pad with zeros; if too long, truncate.
            vals.resize(dims_, 0.0);
        }

        state_ = vals;
        trail_.clear();
        poincarePoints_.clear();

        simulationStarted_ = true;
        simulationActive_ = true;
        if (pauseButton_) {
            pauseButton_->setText(pauseButtonIcon_ + "    " + (simulationActive_ ? "Pause" : "Resume"));
        }
        if (visualizationWidget_) {
            visualizationWidget_->update();
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* e) {
    switch (e->key()) {
    case Qt::Key_1:
        setSystem(1);
        simulationStarted_ = true;
        simulationActive_ = true;
        if (pauseButton_) {
            pauseButton_->setText(pauseButtonIcon_ + "    " + (simulationActive_ ? "Pause" : "Resume"));
        }
        break;
    case Qt::Key_2:
        setSystem(2);
        simulationStarted_ = true;
        simulationActive_ = true;
        if (pauseButton_) {
            pauseButton_->setText(pauseButtonIcon_ + "    " + (simulationActive_ ? "Pause" : "Resume"));
        }
        break;
    case Qt::Key_3:
        setSystem(3);
        simulationStarted_ = true;
        simulationActive_ = true;
        if (pauseButton_) {
            pauseButton_->setText(pauseButtonIcon_ + "    " + (simulationActive_ ? "Pause" : "Resume"));
        }
        break;
    case Qt::Key_4:
        setSystem(4);
        simulationStarted_ = true;
        simulationActive_ = true;
        if (pauseButton_) {
            pauseButton_->setText(pauseButtonIcon_ + "    " + (simulationActive_ ? "Pause" : "Resume"));
        }
        break;
    case Qt::Key_R:
        resetState();
        simulationStarted_ = true;
        simulationActive_ = true;
        if (pauseButton_) {
            pauseButton_->setText(pauseButtonIcon_ + "    " + (simulationActive_ ? "Pause" : "Resume"));
        }
        break;

    case Qt::Key_Plus:
    case Qt::Key_Equal: scale_ *= 1.1; break;
    case Qt::Key_Minus: scale_ /= 1.1; break;

    case Qt::Key_BracketLeft:  // [
        dt_ = std::max(0.001, dt_ / 1.2);
        break;
    case Qt::Key_BracketRight: // ]
        dt_ = std::min(0.05, dt_ * 1.2);
        break;

    case Qt::Key_C: colorMode_ = 1 - colorMode_; break;
    case Qt::Key_F: fadingEnabled_ = !fadingEnabled_; break;
    case Qt::Key_G: gridEnabled_ = !gridEnabled_; break;

    // Cycle draw mode: Trail -> Poincare -> Both -> Trail
    case Qt::Key_O:
        if (drawMode_ == DrawMode::Trail) drawMode_ = DrawMode::Poincare;
        else if (drawMode_ == DrawMode::Poincare) drawMode_ = DrawMode::Both;
        else drawMode_ = DrawMode::Trail;
        break;

    // Adjust trail capacity
    case Qt::Key_T: maxTrail_ += 1000; break;
    case Qt::Key_Y: if (maxTrail_ > 1000) maxTrail_ -= 1000; break;

    case Qt::Key_H:
        // Cycle overlays: None -> PhaseSpace -> Energy -> Lyapunov -> Info -> None
        if (overlayMode_ == OverlayMode::None) overlayMode_ = OverlayMode::PhaseSpace;
        else if (overlayMode_ == OverlayMode::PhaseSpace) overlayMode_ = OverlayMode::Energy;
        else if (overlayMode_ == OverlayMode::Energy) overlayMode_ = OverlayMode::Lyapunov;
        else if (overlayMode_ == OverlayMode::Lyapunov) overlayMode_ = OverlayMode::Info;
        else overlayMode_ = OverlayMode::None;
        break;
    case Qt::Key_I:
        setInitialConditions();
        break;
    case Qt::Key_Space:
        if (simulationStarted_) {
            simulationActive_ = !simulationActive_;
            // Update pause button text
            if (pauseButton_) {
                pauseButton_->setText(pauseButtonIcon_ + "    " + (simulationActive_ ? "Pause" : "Resume"));
            }
        }
        break;
    default: QMainWindow::keyPressEvent(e); break;
    }

    if (visualizationWidget_) {
        visualizationWidget_->update();
    }
}
// CUSTOM IMAGE DRAWING STARTS HERE
void MainWindow::loadCustomImage()
{
    QString filename =
        QFileDialog::getOpenFileName(
            this,
            "Open Image",
            QString(),
            "Images (*.png *.jpg *.jpeg *.bmp *.webp)"
            );

    if (filename.isEmpty())
        return;


    // ---------------------------------------------------------
    // Load image
    // ---------------------------------------------------------

    QImage image(filename);

    if (image.isNull()) {

        QMessageBox::warning(
            this,
            "Invalid Image",
            "Could not load the selected image."
            );

        return;
    }


    // ---------------------------------------------------------
    // Store image
    // ---------------------------------------------------------

    customImage_ =
        image.convertToFormat(
            QImage::Format_RGB32
            );


    // ---------------------------------------------------------
    // Generate the fixed drawing points
    // ---------------------------------------------------------

    generateCustomImagePoints();


    // ---------------------------------------------------------
    // Make sure enough points were found
    // ---------------------------------------------------------

    if (customImagePoints_.size() < 2) {

        QMessageBox::warning(
            this,
            "No Shape Found",
            "Could not find enough visible structure in the image."
            );

        customImage_ = QImage();

        return;
    }


    // ---------------------------------------------------------
    // Activate custom image mode
    // ---------------------------------------------------------

    customImageActive_ = true;

    systemName_ = "Custom Image";

    dims_ = 2;


    // ---------------------------------------------------------
    // Reset animation
    // ---------------------------------------------------------

    customImageDrawIndex_ = 0;

    customImageDrawingFinished_ = false;

    customImageAnimationTime_ = 0.0;


    // ---------------------------------------------------------
    // Clear normal simulation
    // ---------------------------------------------------------

    trail_.clear();

    poincarePoints_.clear();


    // ---------------------------------------------------------
    // Start animation
    // ---------------------------------------------------------

    simulationStarted_ = true;

    simulationActive_ = true;


    if (pauseButton_) {

        pauseButton_->setText(
            pauseButtonIcon_ +
            "    Pause"
            );
    }


    if (visualizationWidget_)
        visualizationWidget_->update();
}

void MainWindow::generateCustomImagePoints()
{
    customImagePoints_.clear();
    customImagePointStrengths_.clear();

    if (customImage_.isNull())
        return;


    // =========================================================
    // SETTINGS
    // =========================================================

    // =========================================================
    // SETTINGS
    // =========================================================

    const int cellSize = 4;   // DEFAULT = 6

    // Maximum number of points.
    const int maxPoints = 25000;  // DEFAULT = 15000

    // Minimum number of points.
    const int minPoints = 10000;  // DEFAULT = 5000


    // =========================================================
    // CREATE GRAYSCALE IMAGE
    // =========================================================

    QImage image =
        customImage_.convertToFormat(
            QImage::Format_Grayscale8
            );

    const int maxProcessingSize = 1200;

    if (image.width() > maxProcessingSize ||
        image.height() > maxProcessingSize) {

        image = image.scaled(
            maxProcessingSize,
            maxProcessingSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            );
    }


    if (image.isNull())
        return;


    const int width =
        image.width();

    const int height =
        image.height();


    if (width < 3 || height < 3)
        return;


    // =========================================================
    // CALCULATE EDGE STRENGTH
    //
    // Sobel-like gradient.
    //
    // This is done ONLY ONCE when the image is generated,
    // NOT every frame during painting.
    // =========================================================

    std::vector<float> edgeStrength(
        static_cast<std::size_t>(width * height),
        0.0f
        );


    float maxEdge =
        0.0f;


    for (int y = 1;
         y < height - 1;
         ++y) {

        for (int x = 1;
             x < width - 1;
             ++x) {


            const int p00 =
                image.constScanLine(y - 1)[x - 1];

            const int p01 =
                image.constScanLine(y - 1)[x];

            const int p02 =
                image.constScanLine(y - 1)[x + 1];


            const int p10 =
                image.constScanLine(y)[x - 1];

            const int p12 =
                image.constScanLine(y)[x + 1];


            const int p20 =
                image.constScanLine(y + 1)[x - 1];

            const int p21 =
                image.constScanLine(y + 1)[x];

            const int p22 =
                image.constScanLine(y + 1)[x + 1];


            // Sobel X
            const int gx =
                -p00
                - 2 * p10
                - p20
                + p02
                + 2 * p12
                + p22;


            // Sobel Y
            const int gy =
                -p00
                - 2 * p01
                - p02
                + p20
                + 2 * p21
                + p22;


            const float magnitude =
                std::sqrt(
                    static_cast<float>(
                        gx * gx +
                        gy * gy
                        )
                    );


            const std::size_t index =
                static_cast<std::size_t>(
                    y * width + x
                    );


            edgeStrength[index] =
                magnitude;


            if (magnitude > maxEdge)
                maxEdge = magnitude;
        }
    }


    if (maxEdge <= 0.0f)
        return;


    // =========================================================
    // NORMALIZE EDGE STRENGTH
    //
    // Slight gamma adjustment makes strong facial/detail
    // edges stand out more.
    // =========================================================

    for (float& value : edgeStrength) {

        value =
            std::clamp(
                value / maxEdge,
                0.0f,
                1.0f
                );


        // Increase separation between weak and strong edges.
        value = std::pow(
            value,
            0.65f
            );
    }


    // =========================================================
    // DETERMINE SAMPLING STEP
    // =========================================================

    // const double totalPixels =
    //     static_cast<double>(
    //         width * height
    //         );


    // double density =
    //     static_cast<double>(
    //         targetPointCount
    //         ) /
    //     totalPixels;


    // density =
    //     std::clamp(
    //         density,
    //         0.01,
    //         0.75
    //         );


    // =========================================================
    // BUILD CANDIDATE POINTS
    //
    // More likely to select strong edges.
    // Weak/background areas are still represented,
    // but much less frequently.
    // =========================================================

    // =========================================================
    // BUILD CANDIDATE POINTS USING SPATIAL CELLS
    //
    // Each cell keeps only its strongest edge point.
    // This prevents large clusters of points in the same
    // small area and greatly reduces the number of points.
    // =========================================================

    // struct Candidate //declared in mainwindow.h
    // {
    //     QPointF point;
    //     float strength;
    // };


    // Number of cells in each direction.
    const int cellsX =
        (width + cellSize - 1) / cellSize;

    const int cellsY =
        (height + cellSize - 1) / cellSize;


    // Store the strongest candidate for every cell.
    //
    // -1 means that the cell has no candidate yet.
    std::vector<int> bestCandidate(
        static_cast<std::size_t>(
            cellsX * cellsY
            ),
        -1
        );


    std::vector<Candidate> candidates;

    candidates.reserve(
        static_cast<std::size_t>(
            cellsX * cellsY
            )
        );


    // ---------------------------------------------------------
    // Find strongest edge in every cell
    // ---------------------------------------------------------

    for (int y = 1;
         y < height - 1;
         ++y) {

        for (int x = 1;
             x < width - 1;
             ++x) {

            const std::size_t index =
                static_cast<std::size_t>(
                    y * width + x
                    );

            const float strength =
                edgeStrength[index];


            // Ignore extremely weak pixels.
            //
            // This is important for photographic backgrounds.
            if (strength < 0.20f)
                continue;


            const int cellX =
                x / cellSize;

            const int cellY =
                y / cellSize;


            const int cellIndex =
                cellY * cellsX + cellX;


            const int existing =
                bestCandidate[
                    static_cast<std::size_t>(
                        cellIndex
                        )
            ];


            // No candidate in this cell yet.
            if (existing == -1) {

                candidates.push_back(
                    {
                        QPointF(
                            static_cast<double>(x) /
                                static_cast<double>(width - 1),

                            static_cast<double>(y) /
                                static_cast<double>(height - 1)
                            ),

                        strength
                    }
                    );


                bestCandidate[
                    static_cast<std::size_t>(
                        cellIndex
                        )
                ] =
                    static_cast<int>(
                        candidates.size() - 1
                        );

            }
            // Replace the existing point if this pixel
            // has a stronger edge.
            else if (
                strength >
                candidates[
                    static_cast<std::size_t>(
                        existing
                        )
            ].strength
                ) {

                candidates[
                    static_cast<std::size_t>(
                        existing
                        )
                ] =
                    {
                        QPointF(
                            static_cast<double>(x) /
                                static_cast<double>(width - 1),

                            static_cast<double>(y) /
                                static_cast<double>(height - 1)
                            ),

                        strength
                    };
            }
        }
    }


    // =========================================================
    // FALLBACK
    //
    // If the image has very little detectable structure,
    // reduce the cell size temporarily to find more points.
    // =========================================================

    if (static_cast<int>(candidates.size()) < minPoints) {

        candidates.clear();

        const int fallbackCellSize =
            std::max(2, cellSize / 2);


        const int fallbackCellsX =
            (width + fallbackCellSize - 1) /
            fallbackCellSize;

        const int fallbackCellsY =
            (height + fallbackCellSize - 1) /
            fallbackCellSize;


        std::vector<int> fallbackBest(
            static_cast<std::size_t>(
                fallbackCellsX *
                fallbackCellsY
                ),
            -1
            );


        for (int y = 1;
             y < height - 1;
             ++y) {

            for (int x = 1;
                 x < width - 1;
                 ++x) {

                const std::size_t index =
                    static_cast<std::size_t>(
                        y * width + x
                        );

                const float strength =
                    edgeStrength[index];


                if (strength < 0.10f)
                    continue;


                const int cx =
                    x / fallbackCellSize;

                const int cy =
                    y / fallbackCellSize;


                const int cellIndex =
                    cy * fallbackCellsX + cx;


                const int existing =
                    fallbackBest[
                        static_cast<std::size_t>(
                            cellIndex
                            )
                ];


                if (existing == -1) {

                    candidates.push_back(
                        {
                            QPointF(
                                static_cast<double>(x) /
                                    static_cast<double>(width - 1),

                                static_cast<double>(y) /
                                    static_cast<double>(height - 1)
                                ),

                            strength
                        }
                        );


                    fallbackBest[
                        static_cast<std::size_t>(
                            cellIndex
                            )
                    ] =
                        static_cast<int>(
                            candidates.size() - 1
                            );

                }
                else if (
                    strength >
                    candidates[
                        static_cast<std::size_t>(
                            existing
                            )
                ].strength
                    ) {

                    candidates[
                        static_cast<std::size_t>(
                            existing
                            )
                    ] =
                        {
                            QPointF(
                                static_cast<double>(x) /
                                    static_cast<double>(width - 1),

                                static_cast<double>(y) /
                                    static_cast<double>(height - 1)
                                ),

                            strength
                        };
                }
            }
        }
    }

    // =========================================================
    // LIMIT TOTAL POINT COUNT
    //
    // IMPORTANT:
    // The entire image has already been scanned.
    // Now keep the strongest points globally.
    // =========================================================

    if (static_cast<int>(candidates.size()) >
        maxPoints) {

        std::partial_sort(
            candidates.begin(),
            candidates.begin() + maxPoints,
            candidates.end(),

            [](const Candidate& a,
               const Candidate& b)
            {
                return a.strength >
                       b.strength;
            }
            );

        candidates.resize(
            maxPoints
            );
    }

    // =========================================================
    // SORT BY EDGE STRENGTH
    //
    // Stronger points are placed together.
    // This also makes the rendering cache-friendly.
    // =========================================================

    // =========================================================
    // STORE FINAL DATA
    // =========================================================

    customImagePoints_.reserve(
        candidates.size()
        );

    customImagePointStrengths_.reserve(
        candidates.size()
        );



    // ============================================================
    // APPLY POINT DENSITY / DISTRIBUTION
    // ============================================================

    applyPointDensityFilter(
        candidates
        );


    // ============================================================
    // COPY FILTERED CANDIDATES
    // ============================================================

    for (const Candidate& candidate :
         candidates) {

        customImagePoints_.push_back(
            candidate.point
            );

        customImagePointStrengths_.push_back(
            candidate.strength
            );
    }


    // =========================================================
    // RESET DRAW INDEX
    // =========================================================

    customImageDrawIndex_ = 0;
}

void MainWindow::applyPointDensityFilter(
    std::vector<Candidate>& candidates)
{
    if (candidates.empty())
        return;


    // ========================================================
    // POINT DENSITY SETTINGS
    // ========================================================

    constexpr double maxSpacing = 0.0055;    //Default = 0.010
    constexpr double minSpacing = 0.0018;   //Default = 0.0035


    // ========================================================
    // STRONGEST FEATURES FIRST
    // ========================================================

    std::sort(
        candidates.begin(),
        candidates.end(),

        [](const Candidate& a,
           const Candidate& b)
        {
            return a.strength >
                   b.strength;
        }
        );


    // ========================================================
    // FILTERED RESULT
    // ========================================================

    std::vector<Candidate> filtered;

    filtered.reserve(
        candidates.size()
        );


    // ========================================================
    // SPATIAL FILTER
    // ========================================================

    for (const Candidate& candidate :
         candidates) {

        const double strength =
            std::clamp(
                static_cast<double>(
                    candidate.strength
                    ),
                0.0,
                1.0
                );


        // Stronger features can have points closer together.
        const double densityStrength =
            std::pow(
                strength,
                0.65
                );


        const double spacing =
            maxSpacing -
            densityStrength *
                (maxSpacing -
                 minSpacing);


        const double spacingSquared =
            spacing * spacing;


        bool tooClose = false;


        for (const Candidate& accepted :
             filtered) {

            const double dx =
                candidate.point.x() -
                accepted.point.x();


            const double dy =
                candidate.point.y() -
                accepted.point.y();


            const double distanceSquared =
                dx * dx +
                dy * dy;


            if (distanceSquared <
                spacingSquared) {

                tooClose = true;
                break;
            }
        }


        if (tooClose)
            continue;


        filtered.push_back(
            candidate
            );
    }


    // ========================================================
    // REPLACE CANDIDATES
    // ========================================================

    candidates.swap(
        filtered
        );
}

void MainWindow::updateVisualizationCenter(const QSize& size)
{
    center_ = QPointF(
        size.width() / 2.0,
        size.height() / 2.0
        );

    formulaNeedsUpdate_ = true;
}


void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    formulaNeedsUpdate_ = true;
}



