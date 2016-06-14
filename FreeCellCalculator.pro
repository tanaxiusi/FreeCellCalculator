TEMPLATE = app
TARGET = FreeCellCalculator

QT += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

LIBS += \
    -lopengl32 \
    -lglu32

HEADERS += ./AStar/AtomicHash.h \
    ./AStar/AreaBase.h \
    ./AStar/Solution.h \
    ./AStar/ChildThread.h \
    ./AStar/WinReadWriteLock.h \
    ./AStar/WinReadWriteLockPool.h \
    ./CommonUI/ProgressDlg.h \
    ./CommonUI/PaintWidgetOpenGL.h \
    ./CommonUI/PaintWidgetNormal.h \
    ./CommonUI/MainDlg.h \
    ./CommonUI/ConfigDlg.h \
    ./Function/Board.h \
    ./Function/BoardArea.h \
    ./Function/Card.h \
    ./Function/CPUTimer.h \
    ./Function/MiniBoard.h \
    ./Function/Shuffle.h \
    ./Function/ThreadAbortHelper.h \
    ./Function/SolutionThread.h \
    ./Function/GamePainter.h \
    ./Util/Other.h
SOURCES += ./main.cpp \
    ./AStar/AtomicHash.cpp \
    ./AStar/AreaBase.cpp \
    ./AStar/WinReadWriteLock.cpp \
    ./AStar/WinReadWriteLockPool.cpp \
    ./CommonUI/ConfigDlg.cpp \
    ./CommonUI/MainDlg.cpp \
    ./CommonUI/PaintWidgetNormal.cpp \
    ./CommonUI/PaintWidgetOpenGL.cpp \
    ./CommonUI/ProgressDlg.cpp \
    ./Function/Board.cpp \
    ./Function/BoardArea.cpp \
    ./Function/Card.cpp \
    ./Function/CPUTimer.cpp \
    ./Function/GamePainter.cpp \
    ./Function/MiniBoard.cpp \
    ./Function/Shuffle.cpp \
    ./Function/SolutionThread.cpp \
    ./Function/ThreadAbortHelper.cpp \
    ./Util/Other.cpp
FORMS += ./ConfigDlg.ui \
    ./MainDlg.ui \
    ./ProgressDlg.ui
RESOURCES += GlobalRes.qrc
