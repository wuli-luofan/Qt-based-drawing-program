#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QColor>
#include <QPen>
#include <QMouseEvent>
#include <QSlider>
#include <QPushButton>
#include <QToolBar>
#include <QLabel>
#include <QComboBox>
#include <QMap>
#include <QLineEdit>
#include <QGraphicsTextItem>
#include <QStack>
#include <QPainterPath>
#include <QGraphicsPathItem>
#include <QVector>
#include <QGraphicsPolygonItem>
#include <QPolygonF>

// 绘图工具枚举
enum class DrawingTool {
    PEN,        // 画笔
    LINE,       // 直线
    RECTANGLE,  // 矩形
    CIRCLE,     // 圆形
    TRIANGLE,   // 三角形
    TEXT        // 文本
};

// 自定义绘图视图
class DrawingView : public QGraphicsView {
    Q_OBJECT
public:
    explicit DrawingView(QGraphicsScene *scene, QWidget *parent = nullptr);
signals:
    void mouseMoved(QPointF scenePos);       // 鼠标移动信号
    void mouseClicked(QPointF scenePos);     // 鼠标点击信号
    void itemDrawn(QGraphicsItem *item);     // 图形绘制完成信号
public slots:
    void setPenColor(const QColor &color);   // 设置画笔颜色
    void setPenWidth(int width);             // 设置画笔粗细
    void setEraserMode(bool isEraser);       // 切换橡皮擦模式
    void setCurrentTool(DrawingTool tool);   // 设置当前工具
    void setTextProperties(const QString &text, int fontSize); // 设置文本属性
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    bool isDrawing;              // 是否正在绘图
    QPointF lastPoint;           // 上一次鼠标位置
    QColor currentColor;         // 当前画笔颜色
    int penWidth;                // 画笔粗细
    bool isEraserMode;           // 是否为橡皮擦模式
    QColor lastColor;            // 切换橡皮擦前的颜色
    DrawingTool currentTool;     // 当前绘图工具
    QGraphicsItem *tempItem;     // 临时绘图项（预览用）
    QString currentText;         // 当前文本内容
    int currentFontSize;         // 当前字体大小
    QPainterPath currentPath;    // 画笔路径
    QVector<QPointF> trianglePoints; // 三角形顶点
};

// 主窗口类
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void initToolBar();                          // 初始化工具栏
    void changeColor(int value);                 // 色相滑块改变颜色
    void changeWidth(int value);                 // 粗细滑块改变粗细
    void toggleEraser();                         // 切换橡皮擦/画笔
    void clearCanvas();                          // 清空画布
    void onMouseMoved(QPointF scenePos);         // 鼠标移动更新状态栏
    void onMouseClicked(QPointF scenePos);       // 鼠标点击处理
    void onToolSelected(int index);              // 切换绘图工具
    void saveAsImage();                          // 保存图片
    void onColorButtonClicked();                 // 颜色按钮点击
    void onTextChanged(const QString &text);     // 文本输入变化
    void onFontSizeChanged(int size);            // 字体大小变化
    void onUndoClicked();                        // 撤回操作
    void onItemDrawn(QGraphicsItem *item);       // 接收绘制完成的图形
private:
    void createColorButtons();                   // 创建颜色按钮
    QWidget* createToolRow1();                   // 创建工具栏第一行
    QWidget* createToolRow2();                   // 创建工具栏第二行

    QGraphicsScene *scene;                       // 绘图场景
    DrawingView *view;                           // 自定义绘图视图
    QToolBar *toolBar;                           // 工具栏

    QComboBox *toolComboBox;                     // 工具选择下拉框
    QSlider *colorSlider;                        // 色相滑块
    QSlider *widthSlider;                        // 粗细滑块
    QPushButton *eraserBtn;                      // 橡皮擦按钮
    QPushButton *clearBtn;                       // 清空按钮
    QPushButton *saveBtn;                        // 保存按钮
    QPushButton *undoBtn;                        // 撤回按钮
    QLabel *colorValueLabel;                     // 色相值显示
    QLabel *widthValueLabel;                     // 粗细值显示

    QWidget *colorButtonsWidget;                 // 颜色按钮容器
    QMap<QPushButton*, QColor> colorButtonMap;   // 按钮-颜色映射

    QLineEdit *textInput;                        // 文本输入框
    QSlider *fontSizeSlider;                     // 字体大小滑块
    QLabel *fontSizeLabel;                       // 字体大小显示
    QString currentText;                         // 当前文本
    int currentFontSize;                         // 当前字体大小

    QStack<QGraphicsItem*> drawingStack;          // 绘制历史栈

    QColor currentColor;                         // 当前颜色
    int penWidth;                                // 当前粗细
    bool isEraserMode;                           // 橡皮擦模式状态
    QColor lastColor;                            // 切换橡皮擦前的颜色
    DrawingTool currentTool;                     // 当前工具
};

#endif // MAINWINDOW_H
