# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line.
SPHINXOPTS    =
SPHINXBUILD   = sphinx-build
SOURCEDIR     = source
BUILDDIR      = build


# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

down:
	wget -O kernel-doc "https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git/plain/scripts/kernel-doc?h=v4.14.52" && chmod +x kernel-doc

genrst:
	 mkdir -p $(BUILDDIR)
	 ./kernel-doc -rst source/pmd_callbacks.h > build/pmd_callbacks.rst
	 ./kernel-doc -rst source/rte_pmd_qdma.h > build/rte_pmd_qdma.rst

html: down genrst
	$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

clean:
	rm -rf $(BUILDDIR)
	rm -rf kernel-doc
