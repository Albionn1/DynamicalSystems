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
#include <QProgressDialog>
#include <QPointer>
#include <QThread>
#include <QElapsedTimer>
#include <QMetaObject>

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <vector>
#include <thread>
#include <functional>

// Parallel worker utility for row-based parallelization across CPU cores
template <typename Func>
static void parallelForRows(int start, int end, Func&& func)
{
    int numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    int total = end - start;
    if (total <= 0) return;
    numThreads = std::min(numThreads, total);

    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    int chunkSize = total / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int rStart = start + i * chunkSize;
        int rEnd = (i == numThreads - 1) ? end : rStart + chunkSize;
        threads.emplace_back(func, rStart, rEnd);
    }
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}

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

                    // Custom Image drawing animation
                    if (customImageProcessing_) {
                        if (visualizationWidget_)
                            visualizationWidget_->update();
                        return;
                    }

                    if (customImageActive_) {
                        if (!customImageDrawingFinished_) {
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

                    // Normal dynamical-system simulation
                    for (int i = 0; i < substeps_; ++i) {
                        step();
                    }

                    if (visualizationWidget_)
                        visualizationWidget_->update();
                }
            });

    timer_.start(16);
}

// =========================================================
// DESTRUCTOR
// =========================================================

MainWindow::~MainWindow()
{
    if (customImageProcessingThread_) {
        customImageProcessingThread_->requestInterruption();
        customImageProcessingThread_->wait();
        customImageProcessingThread_->disconnect();
        delete customImageProcessingThread_;
        customImageProcessingThread_ = nullptr;
    }
}

void MainWindow::createSidebar()
{
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    sidebar_ = new QFrame(centralWidget);
    sidebar_->setFixedWidth(250);

    sidebar_->setStyleSheet(
        "QFrame#sidebar {"
        "    background-color: #202124;"
        "    border-right: 1px solid #34363a;"
        "}"
        );

    sidebar_->setObjectName("sidebar");

    sidebarLayout_ = new QVBoxLayout(sidebar_);
    sidebarLayout_->setContentsMargins(18, 24, 18, 20);
    sidebarLayout_->setSpacing(8);

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

    auto* brandSubtitle = new QLabel("DYNAMICAL SYSTEMS", sidebar_);
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

    auto* simulationLabel = new QLabel("SIMULATION", sidebar_);
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

    createSidebarButton("Save Image", "▣", saveAction_, sidebarLayout_);
    createSidebarButton("Pause", "Ⅱ", pauseAction_, sidebarLayout_);
    createSidebarButton("Reset", "↻", resetAction_, sidebarLayout_);

    sidebarLayout_->addSpacing(18);

    auto* systemsLabel = new QLabel("SYSTEMS", sidebar_);
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

    auto* customImageButton = new QPushButton(sidebar_);
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

    connect(customImageButton, &QPushButton::clicked,
            this, &MainWindow::loadCustomImage);

    sidebarLayout_->addWidget(customImageButton);

    auto* analysisLabel = new QLabel("ANALYSIS", sidebar_);
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

    createSidebarButton("Both Directions", "↔", bothDirectionsAction_, sidebarLayout_);
    createSidebarButton("Keyboard Shortcuts", "⌘", helpAction_, sidebarLayout_);

    sidebarLayout_->addStretch();

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

    auto* infoTitle = new QLabel("QUICK CONTROLS", infoFrame);
    infoTitle->setStyleSheet(
        "QLabel {"
        "    color: #00bcd4;"
        "    font-size: 9px;"
        "    font-weight: 700;"
        "    letter-spacing: 1px;"
        "}"
        );
    infoLayout->addWidget(infoTitle);

    auto* systemInfoLabel = new QLabel(
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

    auto* versionLabel = new QLabel("AKSIOMA  •  v1.0", sidebar_);
    versionLabel->setStyleSheet(
        "QLabel {"
        "    color: #4f5258;"
        "    font-size: 8px;"
        "    letter-spacing: 1px;"
        "}"
        );
    versionLabel->setAlignment(Qt::AlignCenter);
    sidebarLayout_->addWidget(versionLabel);

    mainLayout->addWidget(sidebar_);
    setCentralWidget(centralWidget);
}

void MainWindow::createVisualizationWidget()
{
    QWidget* central = this->centralWidget();
    if (!central) return;

    auto* mainLayout = qobject_cast<QHBoxLayout*>(central->layout());
    if (!mainLayout) return;

    visualizationWidget_ = new VisualizationWidget(this, central);
    visualizationWidget_->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
        );

    mainLayout->addWidget(visualizationWidget_, 1);

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

    if (action->isCheckable()) {
        button->setCheckable(true);
        button->setChecked(action->isChecked());

        connect(button, &QPushButton::toggled, action, &QAction::setChecked);
        connect(action, &QAction::toggled, button, &QPushButton::setChecked);
    } else {
        connect(button, &QPushButton::clicked, action, &QAction::triggered);
    }

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

    if (customImageActive_) {
        customImageDrawIndex_ = 0;
        customImageDrawingFinished_ = false;
        customImageAnimationTime_ = 0.0;
    }

    if (dims_ == 3) {
        state_ = {0.0, 1.0, 20.0};
        Vec tmp = state_;
        double sumz = 0.0;
        const int warmSteps = 1000;

        for (int i = 0; i < warmSteps; ++i) {
            rk4_step(system_, tmp, dt_);
            sumz += tmp[2];
        }

        poincarePlane_ = sumz / double(warmSteps);
    } else if (dims_ == 2) {
        state_ = {1.0, 0.0};
    } else if (dims_ == 4) {
        state_ = {M_PI / 2.0, 0.0, M_PI / 2.0 + 0.1, 0.0};
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

    if (dims_ == 3 && (drawMode_ == DrawMode::Poincare || drawMode_ == DrawMode::Both)) {
        for (auto& c : poincarePoints_) {
            c.age++;
        }
        while (!poincarePoints_.empty() && poincarePoints_.front().age > 300) {
            poincarePoints_.pop_front();
        }

        double prevZ = prev[2];
        double newZ  = state_[2];

        double plane = poincarePlane_;
        bool upwardCross   = (prevZ < plane && newZ >= plane);
        bool downwardCross = (prevZ > plane && newZ <= plane);

        if (upwardCross || (poincareBothDirections_ && downwardCross)) {
            QPointF secPt(center_.x() + state_[0] * scale_,
                          center_.y() - state_[1] * scale_);

            poincarePoints_.emplace_back(secPt, upwardCross);

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
        updateLyapunov();
    }
}

QPointF MainWindow::project(const Vec& x)
{
    if (dims_ == 3) {
        return QPointF(center_.x() + x[0] * scale_, center_.y() - x[1] * scale_);
    }

    if (dims_ == 2) {
        return QPointF(center_.x() + x[0] * scale_, center_.y() - x[1] * scale_);
    }

    if (dims_ == 4) {
        const double th1 = x[0];
        const double th2 = x[2];

        const double x1 = L1_ * std::sin(th1);
        const double y1 = -L1_ * std::cos(th1);

        const double x2 = x1 + L2_ * std::sin(th2);
        const double y2 = y1 - L2_ * std::cos(th2);

        return QPointF(center_.x() + x2 * scale_, center_.y() + y2 * scale_);
    }

    return center_;
}

void MainWindow::paintVisualization(QPainter* p, const QRect& rect)
{
    QLinearGradient bg(rect.topLeft(), rect.bottomLeft());
    bg.setColorAt(0.0, QColor(30, 30, 35));
    bg.setColorAt(0.5, QColor(25, 25, 30));
    bg.setColorAt(1.0, QColor(20, 20, 25));

    p->fillRect(rect, bg);
    p->setRenderHint(QPainter::Antialiasing, true);

    if (gridEnabled_) {
        p->setPen(QPen(QColor(50, 50, 55), 1));
        for (int x = 0; x < rect.width(); x += 50) {
            p->drawLine(x, 0, x, rect.height());
        }
        for (int y = 0; y < rect.height(); y += 50) {
            p->drawLine(0, y, rect.width(), y);
        }
    }

    if (customImageActive_ && !customImage_.isNull()) {
        const bool comparisonMode = customImageDrawingFinished_;
        QRect targetRect;

        if (!comparisonMode) {
            QSize imageSize = customImage_.size();
            const double maxWidth = rect.width() * 0.70;
            const double maxHeight = rect.height() * 0.70;

            const double scaleFactor = std::min(
                maxWidth / static_cast<double>(imageSize.width()),
                maxHeight / static_cast<double>(imageSize.height())
                );

            QSize targetSize(
                static_cast<int>(imageSize.width() * scaleFactor),
                static_cast<int>(imageSize.height() * scaleFactor)
                );

            targetRect = QRect(
                static_cast<int>(center_.x() - targetSize.width() / 2.0),
                static_cast<int>(center_.y() - targetSize.height() / 2.0),
                targetSize.width(),
                targetSize.height()
                );

        } else {
            const int margin = 30;
            const int halfWidth = rect.width() / 2;
            const int availableWidth = halfWidth - margin * 2;
            const int availableHeight = rect.height() - margin * 2;

            const double imageAspect = static_cast<double>(customImage_.width()) / static_cast<double>(customImage_.height());

            int imageWidth = availableWidth;
            int imageHeight = static_cast<int>(imageWidth / imageAspect);

            if (imageHeight > availableHeight) {
                imageHeight = availableHeight;
                imageWidth = static_cast<int>(imageHeight * imageAspect);
            }

            targetRect = QRect(
                margin + (availableWidth - imageWidth) / 2,
                (rect.height() - imageHeight) / 2,
                imageWidth,
                imageHeight
                );
        }

        if (!comparisonMode) {
            p->save();
            p->setOpacity(0.15);
            p->drawImage(targetRect, customImage_);
            p->restore();
        } else {
            const int margin = 30;
            const int halfWidth = rect.width() / 2;
            const int availableWidth = halfWidth - margin * 2;
            const int availableHeight = rect.height() - margin * 2;

            const double imageAspect = static_cast<double>(customImage_.width()) / static_cast<double>(customImage_.height());

            int imageWidth = availableWidth;
            int imageHeight = static_cast<int>(imageWidth / imageAspect);

            if (imageHeight > availableHeight) {
                imageHeight = availableHeight;
                imageWidth = static_cast<int>(imageHeight * imageAspect);
            }

            QRect originalRect(
                halfWidth + (availableWidth - imageWidth) / 2,
                (rect.height() - imageHeight) / 2,
                imageWidth,
                imageHeight
                );

            p->save();
            p->setOpacity(0.50);
            p->drawImage(originalRect, customImage_);
            p->restore();

            p->setPen(QPen(QColor(70, 70, 80), 1));
            p->drawLine(halfWidth, 20, halfWidth, rect.height() - 20);

            p->setRenderHint(QPainter::Antialiasing, true);
            p->setPen(QColor(220, 220, 230));
            p->setFont(QFont("Monospace", 10, QFont::Bold));

            p->drawText(targetRect.left(), targetRect.top() + 30, "POINT RECONSTRUCTION");
            p->drawText(originalRect.left(), originalRect.top() + 30, "ORIGINAL IMAGE");
        }

        const std::size_t pointCount = std::min(customImagePoints_.size(), customImagePointStrengths_.size());
        const std::size_t count = std::min(customImageDrawIndex_, pointCount);

        // --- OPTIMIZED CACHED POINT RENDERING ---
        if (count > 0) {
            // Reset buffer if target size changes or if point generation was restarted
            if (customImageBuffer_.isNull() ||
                customImageBuffer_.size() != targetRect.size() ||
                customImageDrawIndex_ < lastDrawnIndex_)
            {
                customImageBuffer_ = QPixmap(targetRect.size());
                customImageBuffer_.fill(Qt::transparent);
                lastDrawnIndex_ = 0;
            }

            // Draw only new points onto the offscreen buffer with softened edge contrast
            if (count > lastDrawnIndex_) {
                QPainter bufPainter(&customImageBuffer_);
                bufPainter.setRenderHint(QPainter::Antialiasing, true);
                bufPainter.setPen(Qt::NoPen);

                for (std::size_t i = lastDrawnIndex_; i < count; ++i) {
                    const float strength = customImagePointStrengths_[i];
                    if (strength < 0.02f) continue;

                    const QPointF& point = customImagePoints_[i];
                    const QPointF localPoint(
                        point.x() * targetRect.width(),
                        point.y() * targetRect.height()
                        );

                    // Linear strength mapping avoids sharp opacity spikes along image contours
                    const double visualStrength = std::clamp(static_cast<double>(strength), 0.0, 1.0);

                    // Lowered maximum opacity ceiling (capped at ~160) so edge points don't starkly jump out
                    const int alpha = static_cast<int>(35.0 + visualStrength * 125.0);

                    // Tighter radius scaling ensures dense edge areas blend seamlessly without clumping
                    const double radius = 0.85 + visualStrength * 0.5;

                    // Muted, low-contrast slate grey to eliminate harsh brightness and eye strain
                    bufPainter.setBrush(QColor(170, 175, 180, alpha)); //default 170, 175, 180, alpha
                    bufPainter.drawEllipse(localPoint, radius, radius);
                }

                lastDrawnIndex_ = count;
            }
            // Blit cached image buffer to screen in a single operation
            p->drawPixmap(targetRect.topLeft(), customImageBuffer_);
        }

        p->setRenderHint(QPainter::Antialiasing, true);
        p->setPen(QColor(220, 220, 230));
        p->setFont(QFont("Monospace", 10));

        const std::size_t visibleCount = std::min(customImageDrawIndex_, pointCount);
        p->drawText(10, 20, QString("System: Custom Image | Points: %1/%2").arg(visibleCount).arg(pointCount));

        return;
    }

    if ((drawMode_ == DrawMode::Trail || drawMode_ == DrawMode::Both) && trail_.size() > 1) {
        for (int i = 1; i < trail_.size(); ++i) {
            double t = double(i) / trail_.size();
            QColor c;

            if (colorMode_ == 0) {
                c = QColor::fromHsvF(
                    t, 1.0, 1.0,
                    fadingEnabled_ ? (0.2 + 0.8 * (1.0 - t)) : 1.0
                    );
            } else {
                double dx = trail_[i].x() - trail_[i - 1].x();
                double dy = trail_[i].y() - trail_[i - 1].y();
                double speed = std::sqrt(dx * dx + dy * dy);
                double s = std::min(speed / 10.0, 1.0);

                c = QColor::fromHsvF(
                    0.3 + 0.7 * s, 1.0, 1.0,
                    fadingEnabled_ ? (0.2 + 0.8 * (1.0 - t)) : 1.0
                    );
            }

            p->setPen(QPen(c, 2));
            p->drawLine(trail_[i - 1], trail_[i]);
        }
    }

    if (drawMode_ == DrawMode::Poincare || drawMode_ == DrawMode::Both) {
        for (const auto& crossing : poincarePoints_) {
            QColor col = crossing.upward ? QColor("#00aaff") : QColor("#ffffff");
            int alpha = static_cast<int>(255 * std::exp(-crossing.age * 0.005));
            col.setAlpha(alpha);

            p->setPen(Qt::NoPen);
            p->setBrush(col);
            p->drawEllipse(crossing.pos, 3, 3);

            QColor halo = col;
            halo.setAlpha(alpha / 3);
            p->setBrush(halo);
            p->drawEllipse(crossing.pos, 6, 6);
        }
    }

    p->setPen(QColor(220, 220, 230));
    p->setFont(QFont("Monospace", 10));

    int hudTop = 20;

    p->drawText(
        10, hudTop,
        QString("System: %1 | dt=%2 | trail=%3/%4 | substeps=%5 | mode=%6")
            .arg(systemName_)
            .arg(dt_)
            .arg(trail_.size())
            .arg(maxTrail_)
            .arg(substeps_)
            .arg(
                drawMode_ == DrawMode::Trail ? "Trail" :
                    drawMode_ == DrawMode::Poincare ? "Poincaré" : "Both"
                )
        );

    hudTop += 20;

    if (poincareEnabled_ && (drawMode_ == DrawMode::Poincare || drawMode_ == DrawMode::Both)) {
        p->drawText(10, hudTop, QString("Crossings: %1").arg(poincareBothDirections_ ? "Up + Down" : "Up only"));
        hudTop += 20;

        p->drawText(10, hudTop, QString("Poincaré plane z = %1").arg(poincarePlane_));
        hudTop += 20;

        p->drawText(10, hudTop, "Legend: White = Upward, Blue = Downward");
        hudTop += 20;
    } else if (!poincareEnabled_ && (drawMode_ == DrawMode::Poincare || drawMode_ == DrawMode::Both)) {
        p->drawText(10, hudTop, "Poincaré section not available for this system");
        hudTop += 20;
    }

    switch (overlayMode_) {
    case OverlayMode::None: break;
    case OverlayMode::PhaseSpace: drawPhaseSpace(p); break;
    case OverlayMode::Energy: if (dims_ == 4) drawEnergyOverlay(p); break;
    case OverlayMode::Lyapunov: drawLyapunovOverlay(p); break;
    case OverlayMode::Info: drawInfoOverlay(p); break;
    }

    QString formulaPath;
    if (systemName_ == "Lorenz") {
        formulaPath = ":/images/images/lorenzEquationVector.svg";
    } else if (systemName_ == "Rössler") {
        formulaPath = ":/images/images/RosslerEquationVector.svg";
    } else if (systemName_ == "Van der Pol") {
        formulaPath = ":/images/images/VanDerPolEquationVector.svg";
    } else if (systemName_ == "Double Pendulum") {
        formulaPath = ":/images/images/DoublePendulumEquationVector.svg";
    }

    if (!formulaPath.isEmpty()) {
        QSvgRenderer renderer(formulaPath);
        QSizeF svgSize = renderer.defaultSize();
        if (svgSize.isEmpty()) svgSize = QSizeF(240, 140);

        double scaleFactor = 1.0;
        QSizeF targetSize;
        QRectF target;
        QRectF fullRect;
        QColor bgColor;
        QRectF renderRect;

        if (systemName_ == "Double Pendulum") {
            double maxWidth = rect.width() * 0.70;
            if (svgSize.width() > maxWidth) scaleFactor = maxWidth / svgSize.width();

            targetSize = QSizeF(svgSize.width() * scaleFactor, svgSize.height() * scaleFactor);
            target = QRectF(rect.width() - targetSize.width() - 30, rect.height() - targetSize.height() - 30, targetSize.width(), targetSize.height());
            fullRect = target.adjusted(-10, -10, 10, 10);
            bgColor = QColor(45, 45, 50, 90);
            renderRect = QRectF(10, 10, targetSize.width(), targetSize.height());
        } else {
            double maxWidth = rect.width() * 0.15;
            if (svgSize.width() > maxWidth) scaleFactor = maxWidth / svgSize.width();

            targetSize = QSizeF(svgSize.width() * scaleFactor, svgSize.height() * scaleFactor);
            target = QRectF(rect.width() - targetSize.width() - 20, rect.height() - targetSize.height() - 20, targetSize.width(), targetSize.height());
            fullRect = target.adjusted(-8, -8, 8, 8);
            bgColor = QColor(45, 45, 50, 200);
            renderRect = QRectF(8, 8, targetSize.width(), targetSize.height());
        }

        if (formulaNeedsUpdate_) {
            formulaPixmap_ = QPixmap(fullRect.size().toSize());
            formulaPixmap_.fill(Qt::transparent);

            QPainter pixPainter(&formulaPixmap_);
            pixPainter.fillRect(formulaPixmap_.rect(), bgColor);
            pixPainter.setPen(QPen(QColor(200, 200, 210), 1));
            pixPainter.drawRect(formulaPixmap_.rect().adjusted(1, 1, -1, -1));

            renderer.render(&pixPainter, renderRect);
            formulaNeedsUpdate_ = false;
        }

        p->drawPixmap(fullRect.topLeft(), formulaPixmap_);
    }

    if (!simulationStarted_) {
        p->setFont(QFont("Monospace", 12, QFont::Bold));
        p->setPen(Qt::yellow);
        p->drawText(rect, Qt::AlignCenter, "Press 1–4 to start a system");
    } else if (!simulationActive_) {
        QRect box(rect.width() / 2 - 60, 50, 120, 30);
        p->setBrush(Qt::red);
        p->setPen(Qt::NoPen);
        p->drawRect(box);

        p->setPen(Qt::white);
        p->setFont(QFont("Monospace", 10));
        p->drawText(box, Qt::AlignCenter, "PAUSED");
    }
}

void MainWindow::saveSimulationImage(const QString& filename)
{
    if (!visualizationWidget_) return;

    QPixmap pixmap = visualizationWidget_->grab();
    if (!pixmap.save(filename)) {
        QMessageBox::warning(this, "Save Error", "Could not save the simulation image.");
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

    energyHistory_.clear();
    lyapunovDist_.clear();
    lyapunovInitialized_ = false;

    m1_ = 1.0; m2_ = 1.0;
    L1_ = 1.0; L2_ = 1.0;
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

    double sx = inset.width() / rangeX;
    double sy = inset.height() / rangeY;
    double s = std::min(sx, sy) * 0.9;

    p->setPen(QPen(QColor(120, 120, 130), 1, Qt::DashLine));
    p->drawLine(inset.left(), inset.center().y(), inset.right(), inset.center().y());
    p->drawLine(inset.center().x(), inset.top(), inset.center().x(), inset.bottom());

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
    double th1 = state_[0], w1 = state_[1];
    double th2 = state_[2], w2 = state_[3];

    double x1 = L1_ * std::sin(th1);
    double y1 = -L1_ * std::cos(th1);
    double x2 = x1 + L2_ * std::sin(th2);
    double y2 = y1 - L2_ * std::cos(th2);

    double vx1 = L1_ * w1 * std::cos(th1);
    double vy1 = L1_ * w1 * std::sin(th1);
    double vx2 = vx1 + L2_ * w2 * std::cos(th2);
    double vy2 = vy1 + L2_ * w2 * std::sin(th2);

    double KE = 0.5 * m1_ * (vx1*vx1 + vy1*vy1) + 0.5 * m2_ * (vx2*vx2 + vy2*vy2);
    double PE = m1_ * g_ * (y1) + m2_ * g_ * (y2);

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
    if (!state2_.empty()) state2_[0] += 1e-6;
    lyapunovDist_.clear();
    lyapunovInitialized_ = true;
}

void MainWindow::updateLyapunov() {
    rk4_step(system_, state2_, dt_);

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

        if (vals.size() != static_cast<size_t>(dims_)) {
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

    case Qt::Key_BracketLeft:
        dt_ = std::max(0.001, dt_ / 1.2);
        break;
    case Qt::Key_BracketRight:
        dt_ = std::min(0.05, dt_ * 1.2);
        break;

    case Qt::Key_C: colorMode_ = 1 - colorMode_; break;
    case Qt::Key_F: fadingEnabled_ = !fadingEnabled_; break;
    case Qt::Key_G: gridEnabled_ = !gridEnabled_; break;

    case Qt::Key_O:
        if (drawMode_ == DrawMode::Trail) drawMode_ = DrawMode::Poincare;
        else if (drawMode_ == DrawMode::Poincare) drawMode_ = DrawMode::Both;
        else drawMode_ = DrawMode::Trail;
        break;

    case Qt::Key_T: maxTrail_ += 1000; break;
    case Qt::Key_Y: if (maxTrail_ > 1000) maxTrail_ -= 1000; break;

    case Qt::Key_H:
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
    if (customImageProcessingThread_ &&
        customImageProcessingThread_->isRunning()) {

        QMessageBox::information(this, "Processing", "An image is already being processed.");
        return;
    }

    QString filename = QFileDialog::getOpenFileName(
        this, "Open Image", QString(), "Images (*.png *.jpg *.jpeg *.bmp *.webp)"
        );

    if (filename.isEmpty())
        return;

    QImage image(filename);
    if (image.isNull()) {
        QMessageBox::warning(this, "Invalid Image", "Could not load the selected image.");
        return;
    }

    customImage_ = image.convertToFormat(QImage::Format_RGB32);
    customImageActive_ = true;
    systemName_ = "Custom Image";
    dims_ = 2;

    customImagePoints_.clear();
    customImagePointStrengths_.clear();
    customImageParticlePositions_.clear();
    customImageParticleVelocities_.clear();

    customImageDrawIndex_ = 0;
    customImageDrawingFinished_ = false;
    customImageAnimationTime_ = 0.0;

    trail_.clear();
    poincarePoints_.clear();

    customImageProcessing_ = true;
    simulationStarted_ = true;
    simulationActive_ = true;

    if (pauseButton_) {
        pauseButton_->setText(pauseButtonIcon_ + "    Pause");
    }

    generateCustomImagePoints();
}

void MainWindow::generateCustomImagePoints()
{
    startCustomImageProcessing();
}

void MainWindow::startCustomImageProcessing()
{
    if (customImage_.isNull())
        return;

    if (customImageProcessingThread_ && customImageProcessingThread_->isRunning()) {
        return;
    }

    const quint64 requestId = ++customImageProcessingRequest_;
    const QImage sourceImage = customImage_;

    auto resultPoints = std::make_shared<std::vector<QPointF>>();
    auto resultStrengths = std::make_shared<std::vector<float>>();

    auto* progressDialog = new QProgressDialog("Preparing image...", nullptr, 0, 100, this);
    progressDialog->setWindowTitle("AKSIOMA — Processing Image");
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setAutoClose(false);
    progressDialog->setAutoReset(false);
    progressDialog->setMinimumWidth(420);
    progressDialog->setValue(0);
    progressDialog->show();

    QPointer<QProgressDialog> dialogGuard(progressDialog);
    auto progressTimer = std::make_shared<QElapsedTimer>();
    progressTimer->start();

    QThread* thread = QThread::create(
        [this, sourceImage, resultPoints, resultStrengths, progressTimer, dialogGuard]()
        {
            auto reportProgress = [this, progressTimer, dialogGuard](int progress)
            {
                progress = std::clamp(progress, 0, 100);

                QMetaObject::invokeMethod(this, [progressTimer, progress, dialogGuard]() {
                    if (!dialogGuard) return;

                    dialogGuard->setValue(progress);
                    const double elapsedSeconds = progressTimer->elapsed() / 1000.0;

                    QString elapsedText;
                    if (elapsedSeconds < 60.0) {
                        elapsedText = QString("%1 s").arg(elapsedSeconds, 0, 'f', 1);
                    } else {
                        const int minutes = static_cast<int>(elapsedSeconds / 60.0);
                        const double seconds = elapsedSeconds - minutes * 60.0;
                        elapsedText = QString("%1 min %2 s").arg(minutes).arg(seconds, 0, 'f', 0);
                    }

                    QString remainingText = "Calculating...";
                    if (progress > 0 && progress < 100) {
                        const double estimatedTotal = elapsedSeconds * 100.0 / static_cast<double>(progress);
                        const double remainingSeconds = std::max(0.0, estimatedTotal - elapsedSeconds);

                        if (remainingSeconds < 60.0) {
                            remainingText = QString("%1 s remaining").arg(remainingSeconds, 0, 'f', 1);
                        } else {
                            const int minutes = static_cast<int>(remainingSeconds / 60.0);
                            const double seconds = remainingSeconds - minutes * 60.0;
                            remainingText = QString("%1 min %2 s remaining").arg(minutes).arg(seconds, 0, 'f', 0);
                        }
                    } else if (progress >= 100) {
                        remainingText = "Complete";
                    }

                    dialogGuard->setLabelText(
                        QString("Processing image...\n\n%1% complete\nElapsed: %2\n%3")
                            .arg(progress)
                            .arg(elapsedText)
                            .arg(remainingText)
                        );
                }, Qt::QueuedConnection);
            };

            generateCustomImagePointsWorker(
                sourceImage,
                *resultPoints,
                *resultStrengths,
                reportProgress
                );
        }
        );

    customImageProcessingThread_ = thread;

    connect(thread, &QThread::finished, this, [this, thread, requestId, resultPoints, resultStrengths, dialogGuard]() {
        if (dialogGuard) {
            dialogGuard->setValue(100);
            dialogGuard->setLabelText("Processing complete!");
            dialogGuard->close();
            dialogGuard->deleteLater();
        }

        if (requestId != customImageProcessingRequest_) {
            if (customImageProcessingThread_ == thread) {
                customImageProcessingThread_ = nullptr;
            }
            thread->deleteLater();
            return;
        }

        customImagePoints_ = std::move(*resultPoints);
        customImagePointStrengths_ = std::move(*resultStrengths);

        customImageProcessing_ = false;
        customImageDrawIndex_ = 0;
        customImageDrawingFinished_ = false;
        customImageAnimationTime_ = 0.0;

        if (customImagePoints_.size() < 2) {
            customImageActive_ = false;
            simulationActive_ = false;
            QMessageBox::warning(this, "No Shape Found", "Could not find enough visible structure in the image.");
        } else {
            customImageParticlePositions_ = customImagePoints_;
            customImageParticleVelocities_.assign(customImagePoints_.size(), QPointF(0.0, 0.0));
            simulationStarted_ = true;
            simulationActive_ = true;

            if (pauseButton_) {
                pauseButton_->setText(pauseButtonIcon_ + "    Pause");
            }
        }

        if (customImageProcessingThread_ == thread) {
            customImageProcessingThread_ = nullptr;
        }

        if (visualizationWidget_)
            visualizationWidget_->update();

        thread->deleteLater();
    });

    thread->start();
}

void MainWindow::generateCustomImagePointsWorker(
    const QImage& sourceImage,
    std::vector<QPointF>& outputPoints,
    std::vector<float>& outputStrengths,
    const std::function<void(int)>& progressCallback)
{
    auto reportProgress = [&](int value) {
        if (progressCallback) progressCallback(value);
    };

    reportProgress(0);

    outputPoints.clear();
    outputStrengths.clear();

    if (sourceImage.isNull() || QThread::currentThread()->isInterruptionRequested()) {
        reportProgress(100);
        return;
    }

    const int maxPoints = 500000; //Raise this for more details, default is 75000
    const int minPoints = 15000;
    const int maxProcessingSize = 3000;

    constexpr double edgeWeight = 0.65;
    constexpr double textureWeight = 0.35;

    QImage image = sourceImage.convertToFormat(QImage::Format_Grayscale8);

    if (image.width() > maxProcessingSize || image.height() > maxProcessingSize) {
        image = image.scaled(
            maxProcessingSize, maxProcessingSize,
            Qt::KeepAspectRatio, Qt::SmoothTransformation
            );
    }

    if (image.isNull() || QThread::currentThread()->isInterruptionRequested()) return;

    const int width = image.width();
    const int height = image.height();

    reportProgress(5);
    if (width < 7 || height < 7) return;

    std::vector<float> fineEdges(static_cast<std::size_t>(width * height), 0.0f);
    std::vector<float> mediumEdges(static_cast<std::size_t>(width * height), 0.0f);
    std::vector<float> broadEdges(static_cast<std::size_t>(width * height), 0.0f);

    std::atomic<float> maxFineAtomic{0.0f};
    std::atomic<float> maxMediumAtomic{0.0f};
    std::atomic<float> maxBroadAtomic{0.0f};

    // Parallel Fine Sobel Filter
    parallelForRows(1, height - 1, [&](int startY, int endY) {
        float localMaxFine = 0.0f;
        for (int y = startY; y < endY; ++y) {
            const uchar* previous = image.constScanLine(y - 1);
            const uchar* current  = image.constScanLine(y);
            const uchar* next     = image.constScanLine(y + 1);

            for (int x = 1; x < width - 1; ++x) {
                const int p00 = previous[x - 1], p01 = previous[x], p02 = previous[x + 1];
                const int p10 = current[x - 1],                      p12 = current[x + 1];
                const int p20 = next[x - 1],     p21 = next[x],     p22 = next[x + 1];

                const int gx = -p00 - 2 * p10 - p20 + p02 + 2 * p12 + p22;
                const int gy = -p00 - 2 * p01 - p02 + p20 + 2 * p21 + p22;

                const float magnitude = std::sqrt(static_cast<float>(gx * gx + gy * gy));
                fineEdges[static_cast<std::size_t>(y * width + x)] = magnitude;

                if (magnitude > localMaxFine) localMaxFine = magnitude;
            }
        }
        float prevMax = maxFineAtomic.load();
        while (localMaxFine > prevMax && !maxFineAtomic.compare_exchange_weak(prevMax, localMaxFine));
    });

    if (QThread::currentThread()->isInterruptionRequested()) return;
    reportProgress(20);

    // Parallel Medium Edge Filter
    parallelForRows(2, height - 2, [&](int startY, int endY) {
        float localMaxMedium = 0.0f;
        for (int y = startY; y < endY; ++y) {
            const uchar* currentLine = image.constScanLine(y);
            for (int x = 2; x < width - 2; ++x) {
                const float gx = static_cast<float>(currentLine[x + 2] - currentLine[x - 2]);
                const float gy = static_cast<float>(image.constScanLine(y + 2)[x] - image.constScanLine(y - 2)[x]);
                const float magnitude = std::sqrt(gx * gx + gy * gy);

                mediumEdges[static_cast<std::size_t>(y * width + x)] = magnitude;
                if (magnitude > localMaxMedium) localMaxMedium = magnitude;
            }
        }
        float prevMax = maxMediumAtomic.load();
        while (localMaxMedium > prevMax && !maxMediumAtomic.compare_exchange_weak(prevMax, localMaxMedium));
    });

    if (QThread::currentThread()->isInterruptionRequested()) return;
    reportProgress(30);

    // Parallel Broad Edge Filter
    parallelForRows(4, height - 4, [&](int startY, int endY) {
        float localMaxBroad = 0.0f;
        for (int y = startY; y < endY; ++y) {
            const uchar* currentLine = image.constScanLine(y);
            for (int x = 4; x < width - 4; ++x) {
                const float gx = static_cast<float>(currentLine[x + 4] - currentLine[x - 4]);
                const float gy = static_cast<float>(image.constScanLine(y + 4)[x] - image.constScanLine(y - 4)[x]);
                const float magnitude = std::sqrt(gx * gx + gy * gy);

                broadEdges[static_cast<std::size_t>(y * width + x)] = magnitude;
                if (magnitude > localMaxBroad) localMaxBroad = magnitude;
            }
        }
        float prevMax = maxBroadAtomic.load();
        while (localMaxBroad > prevMax && !maxBroadAtomic.compare_exchange_weak(prevMax, localMaxBroad));
    });

    if (QThread::currentThread()->isInterruptionRequested()) return;
    reportProgress(40);

    const float maxFine   = std::max(maxFineAtomic.load(), 0.0001f);
    const float maxMedium = std::max(maxMediumAtomic.load(), 0.0001f);
    const float maxBroad  = std::max(maxBroadAtomic.load(), 0.0001f);

    // Parallel Normalization
    parallelForRows(0, height, [&](int startY, int endY) {
        for (int y = startY; y < endY; ++y) {
            std::size_t idx = static_cast<std::size_t>(y * width);
            for (int x = 0; x < width; ++x, ++idx) {
                fineEdges[idx]   = std::pow(std::clamp(fineEdges[idx] / maxFine, 0.0f, 1.0f), 0.65f);
                mediumEdges[idx] = std::pow(std::clamp(mediumEdges[idx] / maxMedium, 0.0f, 1.0f), 0.65f);
                broadEdges[idx]  = std::pow(std::clamp(broadEdges[idx] / maxBroad, 0.0f, 1.0f), 0.65f);
            }
        }
    });

    if (QThread::currentThread()->isInterruptionRequested()) return;
    reportProgress(50);

    std::vector<float> edgeStrength(static_cast<std::size_t>(width * height), 0.0f);
    parallelForRows(0, height, [&](int startY, int endY) {
        for (int y = startY; y < endY; ++y) {
            std::size_t idx = static_cast<std::size_t>(y * width);
            for (int x = 0; x < width; ++x, ++idx) {
                const double combined = fineEdges[idx] * 0.45 + mediumEdges[idx] * 0.35 + broadEdges[idx] * 0.20;
                edgeStrength[idx] = static_cast<float>(std::clamp(combined, 0.0, 1.0));
            }
        }
    });

    std::vector<float> textureStrength(static_cast<std::size_t>(width * height), 0.0f);

    // Parallel Texture Map Analysis
    parallelForRows(4, height - 4, [&](int startY, int endY) {
        for (int y = startY; y < endY; ++y) {
            for (int x = 4; x < width - 4; ++x) {
                double sum = 0.0;
                double squaredSum = 0.0;
                int samples = 0;

                for (int oy = -4; oy <= 4; oy += 2) {
                    const uchar* scan = image.constScanLine(y + oy);
                    for (int ox = -4; ox <= 4; ox += 2) {
                        const int val = scan[x + ox];
                        sum += val;
                        squaredSum += static_cast<double>(val * val);
                        ++samples;
                    }
                }

                const double mean = sum / static_cast<double>(samples);
                const double variance = std::max(0.0, squaredSum / static_cast<double>(samples) - mean * mean);
                const double standardDeviation = std::sqrt(variance);

                double texture = std::clamp(standardDeviation / 64.0, 0.0, 1.0);
                texture = std::pow(texture, 0.75);

                const std::size_t index = static_cast<std::size_t>(y * width + x);
                textureStrength[index] = static_cast<float>(texture);
            }
        }
    });

    parallelForRows(0, height, [&](int startY, int endY) {
        for (int y = startY; y < endY; ++y) {
            std::size_t idx = static_cast<std::size_t>(y * width);
            for (int x = 0; x < width; ++x, ++idx) {
                const double combined = textureStrength[idx] * 0.75 + fineEdges[idx] * 0.25;
                textureStrength[idx] = static_cast<float>(std::clamp(combined, 0.0, 1.0));
            }
        }
    });

    std::vector<float> finalStrength(static_cast<std::size_t>(width * height), 0.0f);
    parallelForRows(0, height, [&](int startY, int endY) {
        for (int y = startY; y < endY; ++y) {
            std::size_t idx = static_cast<std::size_t>(y * width);
            for (int x = 0; x < width; ++x, ++idx) {
                const double combined = edgeStrength[idx] * edgeWeight + textureStrength[idx] * textureWeight;
                finalStrength[idx] = static_cast<float>(std::clamp(combined, 0.0, 1.0));
            }
        }
    });

    if (QThread::currentThread()->isInterruptionRequested()) return;
    reportProgress(65);

    const int candidateStep = 2;
    std::vector<Candidate> candidates;
    candidates.reserve(static_cast<std::size_t>(((width - 2) / candidateStep) * ((height - 2) / candidateStep)));

    for (int y = 1; y < height - 1; y += candidateStep) {
        for (int x = 1; x < width - 1; x += candidateStep) {
            const std::size_t index = static_cast<std::size_t>(y * width + x);
            const float strength = finalStrength[index];

            if (strength < 0.05f) continue; //Allows faint lines, subtle gradients, and low-contrast edges to produce points. //lower is more details

            Candidate candidate;
            candidate.point = QPointF(
                static_cast<double>(x) / static_cast<double>(width - 1),
                static_cast<double>(y) / static_cast<double>(height - 1)
                );
            candidate.strength = strength;
            candidate.edgeStrength = edgeStrength[index];

            candidates.push_back(candidate);
        }
    }

    reportProgress(78);

    if (static_cast<int>(candidates.size()) < minPoints) {
        candidates.clear();
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                const std::size_t index = static_cast<std::size_t>(y * width + x);
                const float strength = finalStrength[index];
                if (strength < 0.025f) continue;

                Candidate candidate;
                candidate.point = QPointF(
                    static_cast<double>(x) / static_cast<double>(width - 1),
                    static_cast<double>(y) / static_cast<double>(height - 1)
                    );
                candidate.strength = strength;
                candidate.edgeStrength = edgeStrength[index];

                candidates.push_back(candidate);
            }
        }
    }

    constexpr double edgePriorityBoost = 0.22;
    for (Candidate& candidate : candidates) {
        const double strength = std::clamp(static_cast<double>(candidate.strength), 0.0, 1.0);
        const double edge = std::clamp(static_cast<double>(candidate.edgeStrength), 0.0, 1.0);
        const double boost = edge * edgePriorityBoost * (1.0 - strength);

        candidate.strength = static_cast<float>(std::clamp(strength + boost, 0.0, 1.0));
    }

    outputPoints.reserve(std::min(candidates.size(), static_cast<std::size_t>(maxPoints)));
    outputStrengths.reserve(std::min(candidates.size(), static_cast<std::size_t>(maxPoints)));

    if (QThread::currentThread()->isInterruptionRequested()) return;

    // Apply fast O(N) spatial grid density filter
    applyPointDensityFilter(candidates);
    reportProgress(85);

    if (QThread::currentThread()->isInterruptionRequested()) return;

    applyAdaptiveSpatialDistribution(candidates);
    reportProgress(93);

    if (static_cast<int>(candidates.size()) > maxPoints) {
        std::partial_sort(
            candidates.begin(),
            candidates.begin() + maxPoints,
            candidates.end(),
            [](const Candidate& a, const Candidate& b) {
                return a.strength > b.strength;
            }
            );
        candidates.resize(maxPoints);
    }

    reportProgress(97);

    for (const Candidate& candidate : candidates) {
        outputPoints.push_back(candidate.point);
        outputStrengths.push_back(candidate.strength);
    }

    customImageDrawIndex_ = 0;
}

void MainWindow::applyPointDensityFilter(std::vector<Candidate>& candidates)
{
    if (candidates.empty()) return;

    constexpr double maxSpacing = 0.00035; //max space between points, default is: 0.00055
    constexpr double minSpacing = 0.00008; //min space between points, default is: 0.00018

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.strength > b.strength;
    });

    std::vector<Candidate> filtered;
    filtered.reserve(candidates.size());

    // Spatial hash grid optimization: Reduces linear scan O(N^2) to fast grid lookup O(N)
    constexpr int gridDim = 500;
    std::vector<std::vector<int>> spatialGrid(gridDim * gridDim);
    const int cellRadius = std::max(1, static_cast<int>(std::ceil(maxSpacing * gridDim)));

    for (const Candidate& candidate : candidates) {
        const double strength = std::clamp(static_cast<double>(candidate.strength), 0.0, 1.0);
        const double densityStrength = std::pow(strength, 0.65);
        const double spacing = maxSpacing - densityStrength * (maxSpacing - minSpacing);
        const double spacingSquared = spacing * spacing;

        const int cellX = std::clamp(static_cast<int>(candidate.point.x() * gridDim), 0, gridDim - 1);
        const int cellY = std::clamp(static_cast<int>(candidate.point.y() * gridDim), 0, gridDim - 1);

        const int minCellX = std::max(0, cellX - cellRadius);
        const int maxCellX = std::min(gridDim - 1, cellX + cellRadius);
        const int minCellY = std::max(0, cellY - cellRadius);
        const int maxCellY = std::min(gridDim - 1, cellY + cellRadius);

        bool tooClose = false;

        for (int cy = minCellY; cy <= maxCellY && !tooClose; ++cy) {
            for (int cx = minCellX; cx <= maxCellX && !tooClose; ++cx) {
                const auto& cellIndices = spatialGrid[cy * gridDim + cx];
                for (int acceptedIdx : cellIndices) {
                    const Candidate& accepted = filtered[acceptedIdx];
                    const double dx = candidate.point.x() - accepted.point.x();
                    const double dy = candidate.point.y() - accepted.point.y();

                    if (dx * dx + dy * dy < spacingSquared) {
                        tooClose = true;
                        break;
                    }
                }
            }
        }

        if (tooClose) continue;

        const int acceptedIndex = static_cast<int>(filtered.size());
        filtered.push_back(candidate);
        spatialGrid[cellY * gridDim + cellX].push_back(acceptedIndex);
    }

    candidates.swap(filtered);
}

void MainWindow::updateVisualizationCenter(const QSize& size)
{
    center_ = QPointF(size.width() / 2.0, size.height() / 2.0);
    formulaNeedsUpdate_ = true;
}

void MainWindow::applyAdaptiveSpatialDistribution(std::vector<Candidate>& candidates)
{
    if (candidates.empty()) return;

    constexpr int gridWidth  = 100;
    constexpr int gridHeight = 100;
    constexpr double maxSpacing = 0.0030; //max space between points(spatial filter), default was: 0.0050
    constexpr double minSpacing = 0.0002; //min space between points(spatial filter), default was: 0.0012

    struct Cell {
        double strengthSum = 0.0;
        double maxStrength  = 0.0;
        int count           = 0;
    };

    std::vector<Cell> cells(gridWidth * gridHeight);

    for (const Candidate& candidate : candidates) {
        int x = std::clamp(static_cast<int>(candidate.point.x() * gridWidth), 0, gridWidth - 1);
        int y = std::clamp(static_cast<int>(candidate.point.y() * gridHeight), 0, gridHeight - 1);

        Cell& cell = cells[y * gridWidth + x];
        const double strength = std::clamp(static_cast<double>(candidate.strength), 0.0, 1.0);

        cell.strengthSum += strength;
        cell.maxStrength = std::max(cell.maxStrength, strength);
        cell.count++;
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.strength > b.strength;
    });

    std::vector<Candidate> filtered;
    filtered.reserve(candidates.size());

    std::vector<std::vector<int>> spatialGrid(gridWidth * gridHeight);

    for (const Candidate& candidate : candidates) {
        int cellX = std::clamp(static_cast<int>(candidate.point.x() * gridWidth), 0, gridWidth - 1);
        int cellY = std::clamp(static_cast<int>(candidate.point.y() * gridHeight), 0, gridHeight - 1);

        double neighborhoodStrength = 0.0;
        double neighborhoodWeight = 0.0;

        for (int ny = -1; ny <= 1; ++ny) {
            for (int nx = -1; nx <= 1; ++nx) {
                const int neighborX = cellX + nx;
                const int neighborY = cellY + ny;

                if (neighborX < 0 || neighborX >= gridWidth || neighborY < 0 || neighborY >= gridHeight) continue;

                const Cell& neighbor = cells[neighborY * gridWidth + neighborX];
                if (neighbor.count == 0) continue;

                double distance = std::sqrt(static_cast<double>(nx * nx + ny * ny));
                double weight = (distance == 0.0) ? 1.0 : 1.0 / (1.0 + distance);
                double neighborAverage = neighbor.strengthSum / static_cast<double>(neighbor.count);
                double neighborDetail = neighborAverage * 0.70 + neighbor.maxStrength * 0.30;

                neighborhoodStrength += neighborDetail * weight;
                neighborhoodWeight += weight;
            }
        }

        double densityFactor = neighborhoodWeight > 0.0 ? neighborhoodStrength / neighborhoodWeight : 0.0;
        densityFactor = std::pow(std::clamp(densityFactor, 0.0, 1.0), 0.72);

        double spacing = maxSpacing - densityFactor * (maxSpacing - minSpacing);
        const double strength = std::clamp(static_cast<double>(candidate.strength), 0.0, 1.0);

        if (strength > 0.80) spacing *= 0.75;
        else if (strength > 0.65) spacing *= 0.88;

        const double spacingSquared = spacing * spacing;
        bool tooClose = false;

        const int cellRadius = static_cast<int>(std::ceil(spacing * gridWidth));
        const int minCellX = std::max(0, cellX - cellRadius);
        const int maxCellX = std::min(gridWidth - 1, cellX + cellRadius);
        const int minCellY = std::max(0, cellY - cellRadius);
        const int maxCellY = std::min(gridHeight - 1, cellY + cellRadius);

        for (int y = minCellY; y <= maxCellY && !tooClose; ++y) {
            for (int x = minCellX; x <= maxCellX && !tooClose; ++x) {
                const auto& cellPoints = spatialGrid[y * gridWidth + x];
                for (int index : cellPoints) {
                    const Candidate& accepted = filtered[index];
                    const double dx = candidate.point.x() - accepted.point.x();
                    const double dy = candidate.point.y() - accepted.point.y();

                    if (dx * dx + dy * dy < spacingSquared) {
                        tooClose = true;
                        break;
                    }
                }
            }
        }

        if (tooClose) continue;

        const int acceptedIndex = static_cast<int>(filtered.size());
        filtered.push_back(candidate);
        spatialGrid[cellY * gridWidth + cellX].push_back(acceptedIndex);
    }

    candidates.swap(filtered);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    formulaNeedsUpdate_ = true;
}
