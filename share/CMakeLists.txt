# Data: Nothing to build, just installation
install(DIRECTORY . DESTINATION ${DATA_INSTALL_DIR}
  PATTERN "Makefile*" EXCLUDE
  PATTERN "CMake*" EXCLUDE
  PATTERN "cmake*" EXCLUDE
  PATTERN "*~" EXCLUDE
)

install(FILES template/CMakeLists.txt.template DESTINATION ${DATA_INSTALL_DIR} RENAME template/CMakeLists.txt)
