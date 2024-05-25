add_rules("mode.debug", "mode.release")

target("deployqt")
    add_rules("qt.console")
    add_files("src/*.h")
    add_files("src/*.cpp")
    add_files("src/*.qrc")
    
    -- add_files("src/mainwindow.ui")
    -- add files with Q_OBJECT meta (only for qt.moc)
    -- add_files("src/mainwindow.h")
