#include "mainwindow.h"
#include <QStatusBar>
#include <QPainter>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QMessageBox>
#include <QFileDialog>
#include <QPixmap>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFont>
#include <QDir>

// 自定义绘图视图实现
DrawingView::DrawingView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent),
      isDrawing(false),
      currentColor(Qt::black),
      penWidth(3),
      isEraserMode(false),
      lastColor(Qt::black),
      currentTool(DrawingTool::PEN),
      tempItem(nullptr),
      currentFontSize(24),
      currentPath(QPainterPath()) {
    setDragMode(QGraphicsView::NoDrag);
    setRenderHint(QPainter::Antialiasing);
    setMouseTracking(true);
}

// 鼠标按下事件（开始绘图）
void DrawingView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QPointF currentPoint = mapToScene(event->pos());
        QColor drawColor = isEraserMode ? Qt::white : currentColor;
        QPen pen(drawColor, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        QBrush brush(Qt::transparent);

        // 三角形绘制逻辑
        if (currentTool == DrawingTool::TRIANGLE) {
            if (trianglePoints.isEmpty()) {
                trianglePoints.append(currentPoint);
            } else if (trianglePoints.size() == 1) {
                trianglePoints.append(currentPoint);
                isDrawing = true;
                QPolygonF initPolygon({trianglePoints[0], trianglePoints[1], trianglePoints[0]});
                tempItem = new QGraphicsPolygonItem(initPolygon);
                if (auto polygonItem = dynamic_cast<QGraphicsPolygonItem*>(tempItem)) {
                    polygonItem->setPen(pen);
                    polygonItem->setBrush(brush);
                }
                scene()->addItem(tempItem);
            } else if (trianglePoints.size() == 2) {
                trianglePoints.append(currentPoint);
                isDrawing = false;
                QPolygonF finalPolygon(trianglePoints);
                QGraphicsPolygonItem *triangleItem = new QGraphicsPolygonItem(finalPolygon);
                if (auto polygonItem = dynamic_cast<QGraphicsPolygonItem*>(triangleItem)) {
                    polygonItem->setPen(pen);
                    polygonItem->setBrush(brush);
                }
                scene()->addItem(triangleItem);
                emit itemDrawn(triangleItem);
                trianglePoints.clear();
                if (tempItem) {
                    scene()->removeItem(tempItem);
                    delete tempItem;
                    tempItem = nullptr;
                }
            }
            return;
        }

        isDrawing = true;
        lastPoint = currentPoint;
        emit mouseClicked(currentPoint);

        // 文本工具
        if (currentTool == DrawingTool::TEXT && !currentText.isEmpty()) {
            QGraphicsTextItem *textItem = new QGraphicsTextItem(currentText);
            QFont font;
            font.setPointSize(currentFontSize);
            textItem->setFont(font);
            textItem->setDefaultTextColor(drawColor);
            textItem->setPos(currentPoint);
            scene()->addItem(textItem);
            emit itemDrawn(textItem);
            isDrawing = false;
        }
        // 画笔工具
        else if (currentTool == DrawingTool::PEN) {
            currentPath = QPainterPath(currentPoint);
            tempItem = new QGraphicsPathItem(currentPath);
            if (auto pathItem = dynamic_cast<QGraphicsPathItem*>(tempItem)) {
                pathItem->setPen(pen);
            }
            scene()->addItem(tempItem);
        }
        // 其他形状工具
        else if (currentTool != DrawingTool::PEN) {
            if (currentTool == DrawingTool::LINE) {
                tempItem = new QGraphicsLineItem(QLineF(currentPoint, currentPoint));
                if (auto lineItem = dynamic_cast<QGraphicsLineItem*>(tempItem)) {
                    lineItem->setPen(pen);
                }
            } else if (currentTool == DrawingTool::RECTANGLE) {
                tempItem = new QGraphicsRectItem(QRectF(currentPoint, currentPoint));
                if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(tempItem)) {
                    rectItem->setPen(pen);
                    rectItem->setBrush(brush);
                }
            } else if (currentTool == DrawingTool::CIRCLE) {
                tempItem = new QGraphicsEllipseItem(QRectF(currentPoint, currentPoint));
                if (auto ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(tempItem)) {
                    ellipseItem->setPen(pen);
                    ellipseItem->setBrush(brush);
                }
            }
            if (tempItem) {
                scene()->addItem(tempItem);
            }
        }
    }
    QGraphicsView::mousePressEvent(event);
}

// 鼠标移动事件（更新预览）
void DrawingView::mouseMoveEvent(QMouseEvent *event) {
    QPointF currentPoint = mapToScene(event->pos());
    emit mouseMoved(currentPoint);

    if (!isDrawing || !tempItem) return;

    QColor drawColor = isEraserMode ? Qt::white : currentColor;
    QPen pen(drawColor, penWidth, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
    QBrush brush(Qt::transparent);

    // 三角形预览
    if (currentTool == DrawingTool::TRIANGLE && trianglePoints.size() == 2) {
        scene()->removeItem(tempItem);
        delete tempItem;
        QVector<QPointF> previewPoints = trianglePoints;
        previewPoints.append(currentPoint);
        QPolygonF previewPolygon(previewPoints);
        tempItem = new QGraphicsPolygonItem(previewPolygon);
        if (auto polygonItem = dynamic_cast<QGraphicsPolygonItem*>(tempItem)) {
            polygonItem->setPen(pen);
            polygonItem->setBrush(brush);
        }
        scene()->addItem(tempItem);
        return;
    }

    // 其他工具预览
    if (currentTool == DrawingTool::PEN) {
        currentPath.lineTo(currentPoint);
        if (auto pathItem = dynamic_cast<QGraphicsPathItem*>(tempItem)) {
            pathItem->setPath(currentPath);
        }
        lastPoint = currentPoint;
    } else {
        QRectF rect = QRectF(lastPoint, currentPoint).normalized();
        if (currentTool == DrawingTool::LINE) {
            if (auto lineItem = dynamic_cast<QGraphicsLineItem*>(tempItem)) {
                lineItem->setLine(QLineF(lastPoint, currentPoint));
            }
        } else if (currentTool == DrawingTool::RECTANGLE) {
            if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(tempItem)) {
                rectItem->setRect(rect);
            }
        } else if (currentTool == DrawingTool::CIRCLE) {
            if (auto ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(tempItem)) {
                ellipseItem->setRect(rect);
            }
        }
    }
}

// 鼠标释放事件（结束绘图）
void DrawingView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // 三角形特殊处理
        if (currentTool == DrawingTool::TRIANGLE) {
            if (isDrawing) {
                isDrawing = false;
            }
            return;
        }

        isDrawing = false;

        if (currentTool == DrawingTool::PEN && tempItem) {
            emit itemDrawn(tempItem);
            tempItem = nullptr;
            currentPath = QPainterPath();
        } else if (tempItem && currentTool != DrawingTool::TEXT) {
            emit itemDrawn(tempItem);
            tempItem = nullptr;
        }
    }
    QGraphicsView::mouseReleaseEvent(event);
}

// 设置画笔颜色
void DrawingView::setPenColor(const QColor &color) {
    if (!isEraserMode) {
        currentColor = color;
        lastColor = color;
    }
}

// 设置画笔粗细
void DrawingView::setPenWidth(int width) {
    penWidth = width;
}

// 切换橡皮擦模式
void DrawingView::setEraserMode(bool isEraser) {
    isEraserMode = isEraser;
    if (isEraser) {
        lastColor = currentColor;
        currentColor = Qt::white;
    } else {
        currentColor = lastColor;
    }
}

// 设置当前绘图工具
void DrawingView::setCurrentTool(DrawingTool tool) {
    // 切换工具时清理状态
    if (currentTool == DrawingTool::TRIANGLE) {
        trianglePoints.clear();
        if (tempItem) {
            scene()->removeItem(tempItem);
            delete tempItem;
            tempItem = nullptr;
        }
    }
    currentTool = tool;
}

// 设置文本属性
void DrawingView::setTextProperties(const QString &text, int fontSize) {
    currentText = text;
    currentFontSize = fontSize;
}

// 主窗口实现
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      scene(new QGraphicsScene(this)),
      view(new DrawingView(scene, this)),
      toolBar(new QToolBar("绘图工具", this)),
      toolComboBox(new QComboBox()),
      colorSlider(new QSlider(Qt::Horizontal)),
      widthSlider(new QSlider(Qt::Horizontal)),
      eraserBtn(new QPushButton("橡皮擦")),
      clearBtn(new QPushButton("清空画布")),
      saveBtn(new QPushButton("保存图片")),
      undoBtn(new QPushButton("撤回")),
      colorValueLabel(new QLabel("0")),
      widthValueLabel(new QLabel("3px")),
      colorButtonsWidget(new QWidget()),
      textInput(new QLineEdit()),
      fontSizeSlider(new QSlider(Qt::Horizontal)),
      fontSizeLabel(new QLabel("24pt")),
      currentText(""),
      currentFontSize(24),
      drawingStack(QStack<QGraphicsItem*>()),
      currentColor(Qt::black),
      penWidth(3),
      isEraserMode(false),
      lastColor(Qt::black),
      currentTool(DrawingTool::PEN) {


    scene->setSceneRect(0, 0, 800, 600);
    scene->setBackgroundBrush(Qt::white);
    initToolBar();
    setCentralWidget(view);
    setWindowTitle("🌈 彩虹画板）");
    resize(1000, 600);
    statusBar()->showMessage("就绪 - 工具: 画笔 | 历史: 0 项");



    connect(view, &DrawingView::mouseMoved, this, &MainWindow::onMouseMoved);
    connect(view, &DrawingView::mouseClicked, this, &MainWindow::onMouseClicked);
    connect(view, &DrawingView::itemDrawn, this, &MainWindow::onItemDrawn);
}

MainWindow::~MainWindow() {
    qDeleteAll(drawingStack);
    drawingStack.clear();
}

// 初始化工具栏
void MainWindow::initToolBar() {
    addToolBar(Qt::TopToolBarArea, toolBar);
    toolBar->setMovable(false);

    QWidget *mainToolWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainToolWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    mainLayout->addWidget(createToolRow1());
    mainLayout->addWidget(createToolRow2());
    toolBar->addWidget(mainToolWidget);
}

// 创建工具栏第一行
QWidget* MainWindow::createToolRow1() {
    QWidget *rowWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(rowWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    toolComboBox->addItems({"画笔", "直线", "矩形", "圆形", "三角形", "文本"});
    toolComboBox->setCurrentIndex(static_cast<int>(currentTool));
    layout->addWidget(toolComboBox);
    connect(toolComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::onToolSelected);

    layout->addSpacing(15);
    layout->addWidget(new QLabel("常用颜色:"));
    createColorButtons();
    layout->addWidget(colorButtonsWidget);

    layout->addStretch();
    QWidget *colorWidget = new QWidget();
    QHBoxLayout *colorLayout = new QHBoxLayout(colorWidget);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorSlider->setRange(0, 359);
    colorSlider->setValue(0);
    colorValueLabel->setFixedWidth(30);
    colorLayout->addWidget(new QLabel("色相:"));
    colorLayout->addWidget(colorSlider);
    colorLayout->addWidget(colorValueLabel);
    layout->addWidget(colorWidget);
    connect(colorSlider, &QSlider::valueChanged, this, &MainWindow::changeColor);
    connect(colorSlider, &QSlider::valueChanged, this, [this](int value) {
        colorValueLabel->setNum(value);
    });

    return rowWidget;
}

// 创建工具栏第二行
QWidget* MainWindow::createToolRow2() {
    QWidget *rowWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(rowWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    QWidget *widthWidget = new QWidget();
    QHBoxLayout *widthLayout = new QHBoxLayout(widthWidget);
    widthLayout->setContentsMargins(0, 0, 0, 0);
    widthSlider->setRange(1, 20);
    widthSlider->setValue(penWidth);
    widthValueLabel->setFixedWidth(40);
    widthLayout->addWidget(new QLabel("粗细:"));
    widthLayout->addWidget(widthSlider);
    widthLayout->addWidget(widthValueLabel);
    layout->addWidget(widthWidget);
    connect(widthSlider, &QSlider::valueChanged, this, &MainWindow::changeWidth);
    connect(widthSlider, &QSlider::valueChanged, this, [this](int value) {
        widthValueLabel->setText(QString("%1px").arg(value));
    });

    layout->addSpacing(15);
    QWidget *textWidget = new QWidget();
    QHBoxLayout *textLayout = new QHBoxLayout(textWidget);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(5);
    textInput->setPlaceholderText("输入文本...");
    textInput->setMaximumWidth(150);
    fontSizeSlider->setRange(8, 72);
    fontSizeSlider->setValue(currentFontSize);
    fontSizeLabel->setFixedWidth(40);
    textLayout->addWidget(new QLabel("文本:"));
    textLayout->addWidget(textInput);
    textLayout->addWidget(new QLabel("字号:"));
    textLayout->addWidget(fontSizeSlider);
    textLayout->addWidget(fontSizeLabel);
    layout->addWidget(textWidget);
    connect(textInput, &QLineEdit::textChanged, this, &MainWindow::onTextChanged);
    connect(fontSizeSlider, &QSlider::valueChanged, this, &MainWindow::onFontSizeChanged);

    layout->addStretch();
    undoBtn->setEnabled(false);
    layout->addWidget(undoBtn);
    connect(undoBtn, &QPushButton::clicked, this, &MainWindow::onUndoClicked);

    layout->addWidget(eraserBtn);
    layout->addWidget(clearBtn);
    layout->addWidget(saveBtn);
    connect(eraserBtn, &QPushButton::clicked, this, &MainWindow::toggleEraser);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearCanvas);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveAsImage);

    return rowWidget;
}

// 创建常用颜色按钮
void MainWindow::createColorButtons() {
    QHBoxLayout *layout = new QHBoxLayout(colorButtonsWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    QList<QColor> colors = {
        Qt::black, Qt::white, Qt::red, Qt::green, Qt::blue,
        Qt::yellow, Qt::cyan, Qt::magenta, Qt::gray, Qt::darkRed,
        Qt::darkGreen, Qt::darkBlue, Qt::lightGray, QColor(255, 165, 0)
    };

    foreach (const QColor &color, colors) {
        QPushButton *btn = new QPushButton();
        btn->setFixedSize(24, 24);
        QString colorName = (color == QColor(255, 165, 0)) ? "orange" : color.name();
        btn->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: 2px;")
                               .arg(color.name()));
        btn->setToolTip(colorName);
        colorButtonMap.insert(btn, color);
        connect(btn, &QPushButton::clicked, this, &MainWindow::onColorButtonClicked);
        layout->addWidget(btn);
    }
}

// 颜色按钮点击事件
void MainWindow::onColorButtonClicked() {
    QPushButton *clickedBtn = qobject_cast<QPushButton*>(sender());
    if (clickedBtn && colorButtonMap.contains(clickedBtn)) {
        QColor selectedColor = colorButtonMap.value(clickedBtn);
        currentColor = selectedColor;
        lastColor = currentColor;

        if (!isEraserMode) {
            view->setPenColor(currentColor);
        }

        int h, s, v;
        currentColor.getHsv(&h, &s, &v);
        colorSlider->blockSignals(true);
        colorSlider->setValue(h);
        colorValueLabel->setText(QString::number(h));
        colorSlider->blockSignals(false);

        statusBar()->showMessage(QString("颜色已更改至: %1 | 历史: %2 项")
                                     .arg(currentColor.name()).arg(drawingStack.size()));
    }
}

// 文本输入变化事件
void MainWindow::onTextChanged(const QString &text) {
    currentText = text;
    view->setTextProperties(text, currentFontSize);
}

// 字体大小变化事件
void MainWindow::onFontSizeChanged(int size) {
    currentFontSize = size;
    fontSizeLabel->setText(QString("%1pt").arg(size));
    view->setTextProperties(currentText, size);
}

// 色相滑块改变颜色
void MainWindow::changeColor(int value) {
    currentColor = QColor::fromHsv(value, 255, 255);
    lastColor = currentColor;
    if (!isEraserMode) {
        view->setPenColor(currentColor);
    }
    statusBar()->showMessage(QString("颜色: %1 | 历史: %2 项")
                                 .arg(currentColor.name()).arg(drawingStack.size()));
}

// 粗细滑块改变粗细
void MainWindow::changeWidth(int value) {
    penWidth = value;
    view->setPenWidth(value);
    statusBar()->showMessage(QString("粗细: %1px | 历史: %2 项")
                                 .arg(value).arg(drawingStack.size()));
}

// 切换橡皮擦/画笔
void MainWindow::toggleEraser() {
    isEraserMode = !isEraserMode;
    if (isEraserMode) {
        eraserBtn->setText("画笔模式");
        statusBar()->showMessage(QString("橡皮擦模式 | 历史: %1 项")
                                 .arg(drawingStack.size()));
    } else {
        eraserBtn->setText("橡皮擦");
        statusBar()->showMessage(QString("画笔模式 | 历史: %1 项")
                                 .arg(drawingStack.size()));
        view->setPenColor(currentColor);
    }
    view->setEraserMode(isEraserMode);
}

// 绘制完成事件（添加到历史栈）
void MainWindow::onItemDrawn(QGraphicsItem *item) {
    if (item) {
        drawingStack.push(item);
        undoBtn->setEnabled(true);
        statusBar()->showMessage(QString("绘制历史: %1 项").arg(drawingStack.size()));
    }
}

// 撤回操作
void MainWindow::onUndoClicked() {
    if (!drawingStack.isEmpty()) {
        QGraphicsItem *itemToRemove = drawingStack.pop();
        scene->removeItem(itemToRemove);
        delete itemToRemove;
        undoBtn->setEnabled(!drawingStack.isEmpty());
        statusBar()->showMessage(QString("已撤回，剩余历史: %1 项")
                                 .arg(drawingStack.size()));
    }
}

// 清空画布
void MainWindow::clearCanvas() {
    if (QMessageBox::question(this, "确认清空", "是否删除所有绘制内容？",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        qDeleteAll(drawingStack);
        drawingStack.clear();
        scene->clear();
        undoBtn->setEnabled(false);
        statusBar()->showMessage("画布已清空 | 历史: 0 项");
    }
}

// 鼠标移动更新状态栏
void MainWindow::onMouseMoved(QPointF scenePos) {
    QString toolName = toolComboBox->currentText();
    statusBar()->showMessage(QString("工具: %1 | 坐标: (%.1f, %.1f) | 颜色: %2 | 历史: %3 项")
                                 .arg(toolName)
                                 .arg(scenePos.x())
                                 .arg(scenePos.y())
                                 .arg(currentColor.name())
                                 .arg(drawingStack.size()));
}

// 鼠标点击事件
void MainWindow::onMouseClicked(QPointF scenePos) {
    if (currentTool == DrawingTool::TEXT && !currentText.isEmpty()) {
        statusBar()->showMessage(QString("在坐标 (%.1f, %.1f) 添加文本: %1 | 历史: %2 项")
                                     .arg(scenePos.x())
                                     .arg(scenePos.y())
                                     .arg(currentText)
                                     .arg(drawingStack.size()));
    }
}

// 切换绘图工具
void MainWindow::onToolSelected(int index) {
    currentTool = static_cast<DrawingTool>(index);
    view->setCurrentTool(currentTool);
    QString toolName = toolComboBox->currentText();

    bool isTextTool = (currentTool == DrawingTool::TEXT);
    textInput->setEnabled(isTextTool);
    fontSizeSlider->setEnabled(isTextTool);

    statusBar()->showMessage(QString("工具已切换至: %1 | 历史: %2 项")
                                 .arg(toolName).arg(drawingStack.size()));
}

// 保存图片
void MainWindow::saveAsImage() {
    QString filter = "PNG图片 (*.png);;JPG图片 (*.jpg);;BMP图片 (*.bmp)";
    QString filePath = QFileDialog::getSaveFileName(this, "保存图片", QDir::homePath(), filter);
    if (filePath.isEmpty()) return;

    if (!filePath.endsWith(".png", Qt::CaseInsensitive) &&
        !filePath.endsWith(".jpg", Qt::CaseInsensitive) &&
        !filePath.endsWith(".bmp", Qt::CaseInsensitive)) {
        filePath += ".png";
    }

    QPixmap pixmap(scene->sceneRect().size().toSize());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    scene->render(&painter);

    if (pixmap.save(filePath)) {
        statusBar()->showMessage(QString("图片已保存至: %1 | 历史: %2 项")
                                     .arg(filePath).arg(drawingStack.size()));
    } else {
        QMessageBox::warning(this, "保存失败", "无法保存图片，请检查路径是否可写！");
    }
}
