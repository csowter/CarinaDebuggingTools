add_executable(BusRecorderGUI
		FPSMonitor.cpp
		FPSMonitor.h
		GUI.cpp
		GUI.h
		main.cpp
		TimeslotGUIWidgets.cpp
		TimeslotGUIWidgets.h
		)

target_link_libraries(BusRecorderGUI PUBLIC
		CarinaBusDecoder
		sdl-cpp
		SourceCodeProFont
		Serial
		)
