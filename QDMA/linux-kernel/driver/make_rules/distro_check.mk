#
# distro. checks
# uses the same variables calculated from kernel_check.mk
#

distro :=
dmajor :=
dminor :=

# kernel.org rc kernels
#ifneq ($(shell echo $(kextraversion) | $(grep) -c 'git'),0)
#  distro := GIT
#  dmajor :=
#  dminor :=
#endif # kernel.org

ifneq ($(shell $(grep) -c 'RHEL' $(VERSION_H)),0)
  distro := RHEL
#  distro_vmajor := $(shell $(grep) 'RHEL_MAJOR' $(VERSION_H) | cut -d' ' -f3)
#  distro_vminor := $(shell $(grep) 'RHEL_MINOR' $(VERSION_H) | cut -d' ' -f3)
  dmajor := $(word 3, $(shell $(grep) 'RHEL_MAJOR' $(VERSION_H)))
  dminor := $(word 3, $(shell $(grep) 'RHEL_MINOR' $(VERSION_H)))
  ifeq ($(dmajor),)
    ifeq ($(dminor),)
      $(error ERROR! RHEL distro version NOT specified, check version.h.)
    endif
  endif
  ifeq ($(shell [ $(dmajor) -lt 5 ] && echo 1),1)
    $(error ERROR! Unsupported RHEL version $(dmajor).$(dminor).)
  endif
  ifeq ($(dmajor),5)
    ifeq ($(shell [ $(dmajor) -lt 4 ] && echo 1),1)
      $(error ERROR! Unsupported RHEL version $(dmajor).$(dminor).)
    endif
  endif
endif # RHEL 

# SLES does not provide any release macros like RHEL. So we are
# setting Makefile flags for SLES releases based on the version 
# and patchlevel obtained from /etc/SuSE-release file
ifneq ($(wildcard /etc/SuSE-release),)
  distro := SLES
  dmajor := $(shell cat /etc/SuSE-release | grep VERSION | awk '{print $$3}')
  dminor := $(shell cat /etc/SuSE-release | grep PATCHLEVEL | awk '{print $$3}')
endif

$(info "distro=$(distro), dmajor=$(dmajor) dminor=$(dminor) ")

# assume this is kernel.org kernels
ifeq ($(distro),)
  ifeq ($(kseries),2.6)
    ifeq ($(shell [ $(ksublevel) -ge 32 ] && echo 1),1)
      distro := GIT
    else
      $(error kernel version $(kbaseversion)$(kextraversion) NOT supported.)
      $(      kernel.org Requires >= 2.6.32.)
    endif
  endif

  ifeq ($(kversion),3)
    ifeq ($(shell [ $(kpatchlevel) -ge 1 ] && echo 1),1)
      distro := GIT
    else
      $(error kernel version $(kbaseversion)$(kextraversion) NOT supported.)
      $(      kernel.org Requires >= 3.1.)
    endif
  endif

  ifeq ($(kversion),4)
      distro := GIT
  endif

  ifeq ($(kversion),5)
      distro := GIT
  endif
endif # assume kernel.org kernels

ifeq ($(distro),)
  $(error kernel version $(kbaseversion)$(kextraversion) NOT supported.)
  $(      kernel.org Requires >= 2.6.35.)
endif

FLAGS += -D$(distro)$(dmajor)SP$(dminor)
FLAGS += -D$(distro)$(dmajor)
# special case for SLES 11
ifeq ($(distro),SLES)
  ifeq ($(dmajor),11)
    ifeq ($(shell test $(dminor) -ge 1; echo $$?),0)
      FLAGS += -DSLES_RELEASE_11_1
    endif
    ifeq ($(shell test $(dminor) -ge 2; echo $$?),0)
      FLAGS += -DSLES_RELEASE_11_2
    endif
    ifeq ($(shell test $(dminor) -ge 3; echo $$?),0)
      FLAGS += -DSLES_RELEASE_11_3
    endif
    ifeq ($(shell test $(dminor) -ge 4; echo $$?),0)
      FLAGS += -DSLES_RELEASE_11_4
    endif
  else
    FLAGS += -D$(distro)_RELEASE_$(dmajor)_$(dminor)
  endif
endif

$(info $(kbaseversion)$(kextraversion): $(distro),$(dmajor),$(dminor), $(FLAGS))

export distro
export dmajor
export dminor
