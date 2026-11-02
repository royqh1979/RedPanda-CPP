add_executable(qsynedit-multiview-demo main.cpp)

target_link_libraries(qsynedit-multiview-demo PRIVATE
    qsynedit
    Qt::Core
    Qt::Gui
    Qt::Widgets)
