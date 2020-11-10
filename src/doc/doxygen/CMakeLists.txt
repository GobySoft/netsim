file(GLOB FIGURES RELATIVE ${project_SRC_DIR}/doc/figures ${project_SRC_DIR}/doc/figures/*)
foreach(F ${FIGURES})
  configure_file(${project_SRC_DIR}/doc/figures/${F} ${CMAKE_BINARY_DIR}/src/doc/figures/${F} COPYONLY)
endforeach()

configure_file(netsim.doxy.in ${CMAKE_BINARY_DIR}/src/doc/netsim.doxy @ONLY)

file(GLOB MD_FILES ${project_SRC_DIR}/doc/markdown/*.md)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/src/doc/latex/refman.tex ${CMAKE_BINARY_DIR}/src/doc/html/index.html
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/doc/markdown
  COMMAND ${DOXYGEN_EXECUTABLE}
  ARGS ${CMAKE_BINARY_DIR}/src/doc/netsim.doxy
  DEPENDS ${CMAKE_BINARY_DIR}/src/doc/netsim.doxy
  ${MD_FILES}
  ${project_goby_interfaces_figures}
  )


add_custom_target(netsim_doxygen ALL
  DEPENDS ${CMAKE_BINARY_DIR}/src/doc/html/index.html ${CMAKE_BINARY_DIR}/src/doc/latex/refman.pdf
  COMMENT "Running doxygen to generate documentation output to ${CMAKE_BINARY_DIR}/src/doc/html/index.html"
  VERBATIM )

find_package(LATEX QUIET)
if(LATEX_FOUND)

# run pdflatex twice (second pass correctly sets table of contents)
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/src/doc/latex/refman.pdf
  COMMAND bash -c "for i in 1 2; do echo PDFLatex Run $i; ${PDFLATEX_COMPILER} -file-line-error -halt-on-error refman.tex >& /tmp/netsim-doc-pdflatex.log || cat /tmp/netsim-doc-pdflatex.log; done"
  DEPENDS ${CMAKE_BINARY_DIR}/src/doc/latex/refman.tex
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/doc/latex
  VERBATIM )


  add_custom_target(netsim_doxygen_pdf ALL
  DEPENDS ${CMAKE_BINARY_DIR}/src/doc/latex/refman.pdf
  COMMENT "Running doxygen to generate PDF documentation output to ${CMAKE_BINARY_DIR}/src/doc/latex/refman.pdf"
  VERBATIM )
endif()
