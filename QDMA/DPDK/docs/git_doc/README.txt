###############################################################################

               Xilinx QDMA DPDK Driver Documentation Generation

###############################################################################


1. Installation:

	Xilinx QDMA DPDK driver documenation is designed based on Sphinx.
	In oprder to generate the documentation, make sure to install the
	Sphinx software. Details of required packages are available at
	http://www.sphinx-doc.org/en/master/usage/installation.html

	After installing the required packages, follow the steps below to generate the documentation.

	Go to dpdk/docs/git_doc and run 'make html'
  	[xilinx@]# make html
	'build' directory is created. Open 'build/html/index.html' for QDMA DPDK driver documentation

	To remove the generated documentation, run 'make clean'
	[xilinx@]# make clean


