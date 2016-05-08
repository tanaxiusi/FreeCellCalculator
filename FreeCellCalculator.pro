TEMPLATE = app
TARGET = FreeCellCalculator
QT += core gui opengl
CONFIG += release
DEFINES += QT_OPENGL_LIB
INCLUDEPATH += .
DEPENDPATH += .
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
MOC_DIR += ./GeneratedFiles
TRANSLATIONS += ./Translation/en.ts
HEADERS += ./Function/Board.h \
    ./Function/BoardArea.h \
    ./Function/Card.h \
    ./Function/GamePainter.h \
    ./Function/MiniBoard.h \
    ./Function/Shuffle.h \
    ./Function/SolutionThread.h \
    ./Function/ThreadAbortHelper.h \
    ./AStar/AreaBase.h \
    ./AStar/Solution.h \
    ./CommonUI/ConfigDlg.h \
    ./CommonUI/MainDlg.h \
    ./CommonUI/PaintWidgetNormal.h \
    ./CommonUI/PaintWidgetOpenGL.h \
    ./CommonUI/ProgressDlg.h \
    ./Util/Other.h
SOURCES += ./main.cpp \
    ./Function/Board.cpp \
    ./Function/BoardArea.cpp \
    ./Function/Card.cpp \
    ./Function/GamePainter.cpp \
    ./Function/MiniBoard.cpp \
    ./Function/Shuffle.cpp \
    ./Function/SolutionThread.cpp \
    ./Function/ThreadAbortHelper.cpp \
    ./AStar/AreaBase.cpp \
    ./AStar/Solution.cpp \
    ./CommonUI/ConfigDlg.cpp \
    ./CommonUI/MainDlg.cpp \
    ./CommonUI/PaintWidgetNormal.cpp \
    ./CommonUI/PaintWidgetOpenGL.cpp \
    ./CommonUI/ProgressDlg.cpp \
    ./Util/Other.cpp
FORMS += ./ConfigDlg.ui \
    ./MainDlg.ui \
    ./ProgressDlg.ui
TRANSLATIONS += ./Translation/en.ts
RESOURCES += ./GlobalRes.qrc

win32 {
	LIBS += -lopengl32 \
    -lglu32
}

unix {
	QMAKE_LFLAGS += -Wl,-rpath,./
}