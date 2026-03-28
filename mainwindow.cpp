#include "mainwindow.h"
#include "helpdialog.h"
#include "initialconditionsdialog.h"
#include <QPainter>
#include <QKeyEvent>
#include <QFileDialog>
#include <QDateTime>
#include <QPixmap>
#include <QMessageBox>
#include <cmath>
#include <deque>
#include <QSvgRenderer>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Dynamical Systems via ODEs");
    resize(1600, 900);
    center_ = QPointF(width()/2.0, height()/2.0);

    setSystem(1);
    resetState();

    connect(&timer_, &QTimer::timeout, this, [this]{
        // Run simulation steps only when simulation is active
        if (simulationActive_) {
            for (int i = 0; i < substeps_; ++i) step();
            update();
        }
    });
    timer_.start(16);

    // Toolbar
    toolbar_ = new QToolBar("Main Toolbar", this);
    addToolBar(Qt::TopToolBarArea, toolbar_);
    toolbar_->setMovable(false);
    toolbar_->setFloatable(false);
    toolbar_->setStyleSheet(
        "QToolBar { "
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2c3e50, stop:1 #34495e); "
        "    border-bottom: 1px solid #1a252f; "
        "    spacing: 10px; "
        "    padding: 5px; "
        "} "
        "QToolButton { "
        "    background: transparent; "
        "    border: 1px solid transparent; "
        "    border-radius: 6px; "
        "    color: #ecf0f1; "
        "    padding: 8px 16px; "
        "    font-size: 11px; "
        "    font-weight: 500; "
        "} "
        "QToolButton:hover { "
        "    background: rgba(255, 255, 255, 0.1); "
        "    border: 1px solid rgba(255, 255, 255, 0.2); "
        "} "
        "QToolButton:pressed { "
        "    background: rgba(255, 255, 255, 0.2); "
        "    border: 1px solid rgba(255, 255, 255, 0.3); "
        "} "
        "QToolButton:checked { "
        "    background: rgba(52, 152, 219, 0.3); "
        "    border: 1px solid #3498db; "
        "} "
        "QToolButton:checked:hover { "
        "    background: rgba(52, 152, 219, 0.4); "
        "} "
        );

    saveAction_  = toolbar_->addAction("Save Image");
    pauseAction_ = toolbar_->addAction("Pause");
    resetAction_ = toolbar_->addAction("Reset");
    bothDirectionsAction_ = toolbar_->addAction("Both Directions");
    helpAction_ = toolbar_->addAction("Keyboard Shortcuts");
    bothDirectionsAction_->setCheckable(true);
    bothDirectionsAction_->setChecked(false); // default: upward only

    connect(bothDirectionsAction_, &QAction::toggled, this, [this](bool checked){
        poincareBothDirections_ = checked;
    });


    connect(saveAction_, &QAction::triggered, this, [this]{
        QString defaultName = QString("snapshot_%1.png")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
        QString filename = QFileDialog::getSaveFileName(
            this, "Save Simulation Image", defaultName,
            "PNG Images (*.png);;JPEG Images (*.jpg)"
            );
        if (!filename.isEmpty()) saveSimulationImage(filename);
    });

    connect(pauseAction_, &QAction::triggered, this, [this]{
        simulationActive_ = !simulationActive_;
        simulationStarted_ = simulationStarted_ || simulationActive_; // mark started once run
        pauseAction_->setText(simulationActive_ ? "Pause" : "Resume");
    });

    connect(resetAction_, &QAction::triggered, this, [this]{
        resetState();
    });

    connect(helpAction_, &QAction::triggered, this, [this]{
        HelpDialog dlg(this);
        dlg.exec();
    });
}

void MainWindow::resetState() {
    trail_.clear();
    poincarePoints_.clear();
    if (dims_ == 3) {
        state_ = { 0.0, 1.0, 20.0 };
        // Warmup integration to estimate a sensible Poincare plane (mean z)
        Vec tmp = state_;
        double sumz = 0.0;
        const int warmSteps = 1000;
        for (int i = 0; i < warmSteps; ++i) {
            rk4_step(system_, tmp, dt_);
            sumz += tmp[2];
        }
        poincarePlane_ = sumz / double(warmSteps);
    } else if (dims_ == 2) {
        state_ = { 1.0, 0.0 };
    } else if (dims_ == 4) {
        state_ = { M_PI/2.0, 0.0, M_PI/2.0 + 0.01, 0.0 };
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

QPointF MainWindow::project(const Vec& x) {
    if (dims_ == 3) {
        return QPointF(center_.x() + x[0] * scale_, center_.y() - x[1] * scale_);
    } else if (dims_ == 2) {
        return QPointF(center_.x() + x[0] * scale_, center_.y() - x[1] * scale_);
    } else if (dims_ == 4) {
        double L1 = 1.0, L2 = 1.0;
        double th1 = state_[0], th2 = state_[2];
        double x1 = L1 * std::sin(th1);
        double y1 = -L1 * std::cos(th1);
        double x2 = x1 + L2 * std::sin(th2);
        double y2 = y1 - L2 * std::cos(th2);
        return QPointF(center_.x() + x2 * scale_, center_.y() + y2 * scale_);
    }
    return center_;
}

void MainWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(18, 18, 22));
    p.setRenderHint(QPainter::Antialiasing, true);

    if (gridEnabled_) {
        p.setPen(QPen(QColor(60, 60, 70), 1));
        for (int x = 0; x < width(); x += 50) p.drawLine(x, 0, x, height());
        for (int y = 0; y < height(); y += 50) p.drawLine(0, y, width(), y);
    }

    // Trail rendering
    if ((drawMode_ == DrawMode::Trail || drawMode_ == DrawMode::Both) && trail_.size() > 1) {
        for (int i = 1; i < trail_.size(); ++i) {
            double t = double(i) / trail_.size();
            QColor c;
            if (colorMode_ == 0) { // time gradient
                c = QColor::fromHsvF(t, 1.0, 1.0, fadingEnabled_ ? (0.2 + 0.8*(1.0 - t)) : 1.0);
            } else { // speed coloring
                double dx = trail_[i].x() - trail_[i-1].x();
                double dy = trail_[i].y() - trail_[i-1].y();
                double speed = std::sqrt(dx*dx + dy*dy);
                double s = std::min(speed / 10.0, 1.0);
                c = QColor::fromHsvF(0.3 + 0.7*s, 1.0, 1.0, fadingEnabled_ ? (0.2 + 0.8*(1.0 - t)) : 1.0);
            }
            p.setPen(QPen(c, 2));
            p.drawLine(trail_[i-1], trail_[i]);
        }
    }
    //Debugging

    // qDebug() << "z:" << state_[2];
    // qDebug() << "poincarePlane:" << poincarePlane_;


    // Poincare section points
    if (drawMode_ == DrawMode::Poincare || drawMode_ == DrawMode::Both) {
        for (const auto& crossing : poincarePoints_) {
            QColor col = crossing.upward ? QColor("#00aaff")  // for upward
                                         : QColor("#ffffff");  // for downward

            int alpha = static_cast<int>(255 * std::exp(-crossing.age * 0.005));
            col.setAlpha(alpha);

            // inner dot
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            p.drawEllipse(crossing.pos, 3, 3);

            QColor halo = col;
            halo.setAlpha(alpha / 3);
            p.setBrush(halo);
            p.drawEllipse(crossing.pos, 6, 6);
        }
    }

    // HUD
    p.setPen(QColor(220, 220, 230));
    p.setFont(QFont("Monospace", 10));
    int hudTop = toolbar_->height() + 20;

    // Always show system info
    p.drawText(10, hudTop,
               QString("System: %1 | dt=%2 | trail=%3/%4 | substeps=%5 | mode=%6")
                   .arg(systemName_)
                   .arg(dt_)
                   .arg(trail_.size())
                   .arg(maxTrail_)
                   .arg(substeps_)
                   .arg(drawMode_ == DrawMode::Trail ? "Trail" :
                            drawMode_ == DrawMode::Poincare ? "Poincaré" : "Both"));
    hudTop += 20;

    // Only show Poincare info if enabled
    if (poincareEnabled_ && (drawMode_ == DrawMode::Poincare || drawMode_ == DrawMode::Both)) {
        p.drawText(10, hudTop,
                   QString("Crossings: %1")
                       .arg(poincareBothDirections_ ? "Up + Down" : "Up only"));
        hudTop += 20;

        p.drawText(10, hudTop,
                   QString("Poincaré plane z = %1").arg(poincarePlane_));
        hudTop += 20;

        p.drawText(10, hudTop, "Legend: White = Upward, Blue = Downward");
        hudTop += 20;
    } else if (!poincareEnabled_ && (drawMode_ == DrawMode::Poincare || drawMode_ == DrawMode::Both)) {
        p.drawText(10, hudTop, "Poincaré section not available for this system");
        hudTop += 20;
    }

    // Educational overlays
    switch (overlayMode_) {
    case OverlayMode::None:      break;
    case OverlayMode::PhaseSpace: drawPhaseSpace(p);     break;
    case OverlayMode::Energy:     if (dims_ == 4) drawEnergyOverlay(p); break;
    case OverlayMode::Lyapunov:   drawLyapunovOverlay(p); break;
    case OverlayMode::Info:       drawInfoOverlay(p);     break;
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

        // Get natural size of SVG
        QSizeF svgSize = renderer.defaultSize();
        if (svgSize.isEmpty()) svgSize = QSizeF(240, 140); // fallback

        double scaleFactor = 1.0;
        QSizeF targetSize;
        QRectF target;
        QRectF fullRect;
        QColor bgColor;
        QRectF renderRect;

        if (systemName_ == "Double Pendulum") {
            // Allow a wider box for pendulum formulas
            double maxWidth = width() * 0.70;
            if (svgSize.width() > maxWidth) {
                scaleFactor = maxWidth / svgSize.width();
            }
            targetSize = QSizeF(svgSize.width() * scaleFactor,
                                svgSize.height() * scaleFactor);

            target = QRectF(width() - targetSize.width() - 30,
                          height() - targetSize.height() - 30,
                          targetSize.width(),
                          targetSize.height());

            fullRect = target.adjusted(-10,-10,10,10);
            bgColor = QColor(30,30,40,90);
            renderRect = QRectF(10, 10, targetSize.width(), targetSize.height());

        } else {
            // Default sizing for other systems
            double maxWidth = width() * 0.15;   // up to 15% of window width
            if (svgSize.width() > maxWidth) {
                scaleFactor = maxWidth / svgSize.width();
            }
            targetSize = QSizeF(svgSize.width() * scaleFactor,
                                svgSize.height() * scaleFactor);

            target = QRectF(width() - targetSize.width() - 20,
                          height() - targetSize.height() - 20,
                          targetSize.width(),
                          targetSize.height());

            fullRect = target.adjusted(-8,-8,8,8);
            bgColor = QColor(30,30,40,200);
            renderRect = QRectF(8, 8, targetSize.width(), targetSize.height());
        }

        if (formulaNeedsUpdate_) {
            formulaPixmap_ = QPixmap(fullRect.size().toSize());
            formulaPixmap_.fill(Qt::transparent);
            QPainter pixPainter(&formulaPixmap_);
            pixPainter.fillRect(formulaPixmap_.rect(), bgColor);
            pixPainter.setPen(QPen(QColor(200,200,210),1));
            pixPainter.drawRect(formulaPixmap_.rect().adjusted(1,1,-1,-1));
            renderer.render(&pixPainter, renderRect);
            formulaNeedsUpdate_ = false;
        }

        p.drawPixmap(fullRect.topLeft(), formulaPixmap_);
    }
    if (!simulationStarted_) {
        p.setFont(QFont("Monospace", 12, QFont::Bold));
        p.setPen(Qt::yellow);
        p.drawText(rect(), Qt::AlignCenter,
                   "Press 1–4 to start a system");
    }

    else if (!simulationActive_) {
        QRect box(750, 50, 120, 30);
        p.setBrush(Qt::red);
        p.setPen(Qt::NoPen);
        p.drawRect(box);

        p.setPen(Qt::white);
        p.setFont(QFont("Monospace", 10));
        p.drawText(box, Qt::AlignCenter, "PAUSED");
    }

}


void MainWindow::saveSimulationImage(const QString& filename) {
    QPixmap pixmap(size());
    pixmap.fill(QColor(18, 18, 22));

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.setPen(QPen(QColor(60, 60, 70), 1));
    for (int x = 0; x < width(); x += 50) p.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += 50) p.drawLine(0, y, width(), y);

    // Trail (always draw trail in saved image)
    if (trail_.size() > 1) {
        for (int i = 1; i < trail_.size(); ++i) {
            double t = double(i) / trail_.size();
            QColor c;
            if (colorMode_ == 0) {
                c = QColor::fromHsvF(t, 1.0, 1.0, fadingEnabled_ ? (0.2 + 0.8*(1.0 - t)) : 1.0);
            } else {
                double dx = trail_[i].x() - trail_[i-1].x();
                double dy = trail_[i].y() - trail_[i-1].y();
                double speed = std::sqrt(dx*dx + dy*dy);
                double s = std::min(speed / 10.0, 1.0);
                c = QColor::fromHsvF(0.3 + 0.7*s, 1.0, 1.0, fadingEnabled_ ? (0.2 + 0.8*(1.0 - t)) : 1.0);
            }
            p.setPen(QPen(c, 2));
            p.drawLine(trail_[i-1], trail_[i]);
        }
    }

    // Poincare points if mode includes them
    if (drawMode_ == DrawMode::Poincare || drawMode_ == DrawMode::Both) {
        p.setPen(QPen(QColor("#ffff00"), 8));
        for (const auto& pt : poincarePoints_) {
            p.drawPoint(pt.pos);
        }
    }

    pixmap.save(filename);
}

void MainWindow::setSystem(int id) {
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
        scale_ = 120.0;
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

void MainWindow::drawPhaseSpace(QPainter& p) {
    QRectF inset(width() - 300, 50, 250, 250);
    p.fillRect(inset, QColor(30, 30, 40));
    p.setPen(QPen(QColor(200, 200, 210), 1));
    p.drawRect(inset);

    p.setFont(QFont("Monospace", 9));
    p.drawText(inset.left() + 8, inset.top() + 18, "Phase space");

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

    p.setPen(QPen(QColor(120, 120, 130), 1, Qt::DashLine));
    p.drawLine(inset.left(), inset.center().y(), inset.right(), inset.center().y());
    p.drawLine(inset.center().x(), inset.top(), inset.center().x(), inset.bottom());

    // Axis labels
    p.setFont(QFont("Monospace", 8));
    if (dims_ == 4) {
        p.drawText(inset.right() - 20, inset.center().y() - 5, "θ1");
        p.drawText(inset.center().x() + 5, inset.top() + 15, "θ2");
    } else {
        p.drawText(inset.right() - 15, inset.center().y() - 5, "x");
        p.drawText(inset.center().x() + 5, inset.top() + 15, "y");
    }

    p.setPen(QPen(QColor("#ffaa00"), 1));
    for (int i = 1; i < trail_.size(); ++i) {
        QPointF a(inset.left() + (trail_[i-1].x() - minX) * s,
                  inset.bottom() - (trail_[i-1].y() - minY) * s);
        QPointF b(inset.left() + (trail_[i].x() - minX) * s,
                  inset.bottom() - (trail_[i].y() - minY) * s);
        p.drawLine(a, b);
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

void MainWindow::drawEnergyOverlay(QPainter& p) {
    QRectF inset(width() - 300, 50, 250, 250);
    p.fillRect(inset, QColor(30, 30, 40));
    p.setPen(QPen(QColor(200, 200, 210), 1));
    p.drawRect(inset);
    p.setFont(QFont("Monospace", 9));
    p.drawText(inset.left() + 8, inset.top() + 18, "Total energy (KE+PE)");

    if (energyHistory_.size() < 2) return;

    // Normalize to fit
    double minE = *std::min_element(energyHistory_.begin(), energyHistory_.end());
    double maxE = *std::max_element(energyHistory_.begin(), energyHistory_.end());
    double range = std::max(1e-6, maxE - minE);

    p.setPen(QPen(QColor("#66ccff"), 2));
    for (int i = 1; i < energyHistory_.size(); ++i) {
        double t0 = double(i-1) / (energyHistory_.size()-1);
        double t1 = double(i)   / (energyHistory_.size()-1);
        QPointF a(inset.left() + t0 * inset.width(),
                  inset.bottom() - ((energyHistory_[i-1] - minE) / range) * inset.height());
        QPointF b(inset.left() + t1 * inset.width(),
                  inset.bottom() - ((energyHistory_[i]   - minE) / range) * inset.height());
        p.drawLine(a, b);
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

void MainWindow::drawLyapunovOverlay(QPainter& p) {
    QRectF inset(width() - 300, 50, 250, 250);
    p.fillRect(inset, QColor(30, 30, 40));
    p.setPen(QPen(QColor(200, 200, 210), 1));
    p.drawRect(inset);
    p.setFont(QFont("Monospace", 9));
    p.drawText(inset.left() + 8, inset.top() + 18, "Trajectory divergence (|Δstate|)");

    if (lyapunovDist_.size() < 2) return;

    double minD = *std::min_element(lyapunovDist_.begin(), lyapunovDist_.end());
    double maxD = *std::max_element(lyapunovDist_.begin(), lyapunovDist_.end());
    double range = std::max(1e-12, maxD - minD);

    p.setPen(QPen(QColor("#ff6688"), 2));
    for (int i = 1; i < lyapunovDist_.size(); ++i) {
        double t0 = double(i-1) / (lyapunovDist_.size()-1);
        double t1 = double(i)   / (lyapunovDist_.size()-1);
        QPointF a(inset.left() + t0 * inset.width(),
                  inset.bottom() - ((lyapunovDist_[i-1] - minD) / range) * inset.height());
        QPointF b(inset.left() + t1 * inset.width(),
                  inset.bottom() - ((lyapunovDist_[i]   - minD) / range) * inset.height());
        p.drawLine(a, b);
    }
}

void MainWindow::drawInfoOverlay(QPainter& p) {
    QRectF inset(width() - 420, 50, 400, 100);
    p.fillRect(inset, QColor(30, 30, 40, 230));
    p.setPen(QPen(QColor(220, 220, 230), 1));
    p.drawRect(inset);

    p.setFont(QFont("Monospace", 10));
    QString title = QString("%1 — About").arg(systemName_);
    p.drawText(inset.left() + 10, inset.top() + 22, title);

    p.setFont(QFont("Monospace", 9));
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
        p.drawText(inset.left() + 10, y, line);
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
        if (pauseAction_) pauseAction_->setText("Pause");
        update();
    }
}

void MainWindow::keyPressEvent(QKeyEvent* e) {
    switch (e->key()) {
    case Qt::Key_1:
        setSystem(1);
        simulationStarted_ = true;
        simulationActive_ = true;
        if (pauseAction_) pauseAction_->setText("Pause");
        break;
    case Qt::Key_2:
        setSystem(2);
        simulationStarted_ = true;
        simulationActive_ = true;
        if (pauseAction_) pauseAction_->setText("Pause");
        break;
    case Qt::Key_3:
        setSystem(3);
        simulationStarted_ = true;
        simulationActive_ = true;
        if (pauseAction_) pauseAction_->setText("Pause");
        break;
    case Qt::Key_4:
        setSystem(4);
        simulationStarted_ = true;
        simulationActive_ = true;
        if (pauseAction_) pauseAction_->setText("Pause");
        break;
    case Qt::Key_R:
        resetState();
        simulationStarted_ = true;
        simulationActive_ = true;
        if (pauseAction_) pauseAction_->setText("Pause");
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
            if (pauseAction_) pauseAction_->setText(simulationActive_ ? "Pause" : "Resume");
        }
        break;
    default: QMainWindow::keyPressEvent(e); break;
    }

    update();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    center_ = QPointF(width()/2.0, height()/2.0);
    formulaNeedsUpdate_ = true;
}
