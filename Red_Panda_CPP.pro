TEMPLATE = subdirs

SUBDIRS += \
    RedPandaIDE \
    astyle \
    consolepauser

APP_NAME = RedPandaCPP

linux: {

    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }

    QMAKE_SUBSTITUTES += redpandaide.desktop.in

    resources.path = $${PREFIX}/share/$${APP_NAME}
    resources.files += linux/templates
    INSTALLS += resources

    docs.path = $${PREFIX}/share/doc/$${APP_NAME}
    docs.files += README.md
    docs.files += NEWS.md
    docs.files += LICENSE
    INSTALLS += docs

    pixmaps.path = $${PREFIX}/share/pixmaps
    pixmaps.files += linux/redpandaide.png
    INSTALLS += pixmaps

    desktop.path = $${PREFIX}/share/applications
    desktop.files += redpandaide.desktop
    INSTALLS += desktop

}
