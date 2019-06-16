//
// Created by martin on 09/12/18.
//

#ifndef MESSAGES_CALENHADGLOBEDIALOG_H
#define MESSAGES_CALENHADGLOBEDIALOG_H
#include <QMainWindow>
#include "../../qmodule/Module.h"
#include "CalenhadGlobeWidget.h"

namespace calenhad {
    namespace nodeedit {
        class CalenhadToolBar;
    }
    namespace controls {
        namespace globe {
            class CalenhadGlobeDialog : public QMainWindow {
            Q_OBJECT
            public:
                CalenhadGlobeDialog (QWidget* parent, calenhad::qmodule::Module* module);
                virtual ~CalenhadGlobeDialog();
                CalenhadGlobeWidget* widget();
                void initialise();

            protected:
                CalenhadGlobeWidget* _widget;
                calenhad::qmodule::Module* _module;

                void closeEvent (QCloseEvent* e) override;
                void showEvent (QShowEvent* e) override;
                QDockWidget* _mainDock, * _mouseDock, * _widgetDock;

            };

        }
    }
}



#endif //MESSAGES_CALENHADGLOBEDIALOG_H
