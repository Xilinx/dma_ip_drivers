cmake_minimum_required(VERSION 3.5)
set(LINUX_QDMA_SRC_DIR            "${CMAKE_CURRENT_SOURCE_DIR}/")
set(LINUX_QDMA_DOC_TOC_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/toc")
set(LINUX_QDMA_DOC_CORE_DIR       "${CMAKE_CURRENT_BINARY_DIR}/core")
set(DOC_TOC_DIR        			  "${CMAKE_CURRENT_BINARY_DIR}/doc_toc")

file(GLOB LIBQDMA_EXPORT_H ${LINUX_QDMA_SRC_DIR}/../../libqdma/libqdma_export.h)

file(MAKE_DIRECTORY ${LINUX_QDMA_DOC_CORE_DIR})
file(MAKE_DIRECTORY ${DOC_TOC_DIR})
#file(COPY ${LINUX_QDMA_DOC_TOC_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


set(KERNELDOC "./kernel-doc")
set(KERNELDOC_URL "https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git/plain/scripts/kernel-doc?h=v4.14.52")
MESSAGE(STATUS "${KERNELDOC} downloading")
file(DOWNLOAD ${KERNELDOC_URL} ${KERNELDOC})
execute_process(COMMAND chmod +x ${KERNELDOC})
find_program(KERNELDOC_EXECUTABLE ${KERNELDOC} PATHS "./")

find_program(SPHINX_EXECUTABLE sphinx-build)

if (NOT KERNELDOC_EXECUTABLE OR NOT SPHINX_EXECUTABLE)
  MESSAGE (WARNING "kernel-doc or Sphinx not found, XRT documentation build disabled")
else ()
  add_custom_command(OUTPUT core/libqdma_export.inc
	MESSAGE(STATUS "Generating libqdma_export.inc , ${LIBQDMA_EXPORT_H}")
    COMMAND ${KERNELDOC_EXECUTABLE} -rst ${LIBQDMA_EXPORT_H} > core/libqdma_export.inc
    DEPENDS ${LIBQDMA_EXPORT_H}
    VERBATIM
    )
	
  add_custom_target(
    linux_qdma_docs ALL
	COMMENT "Generating documentation with Sphinx"
    DEPENDS core/libqdma_export.inc
    COMMAND mkdir -p html
    COMMAND rm -rf ${DOC_TOC_DIR}/*
	COMMAND cp -rf ${LINUX_QDMA_DOC_TOC_DIR}/* ${DOC_TOC_DIR}/
    COMMAND cp core/libqdma_export.inc ${DOC_TOC_DIR}
    COMMAND ${SPHINX_EXECUTABLE} -a ${DOC_TOC_DIR} html
    )
endif ()

