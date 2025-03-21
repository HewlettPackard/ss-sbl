# Build for Cassini hardware by default; build for emulator or netsim by
# defining the 'platform' macro.
# e.g. rpmbuild --define 'platform PLATFORM_CASSINI_SIM' ...
# If 'platform' is not defined, default to PLATFORM_CASSINI_HW
%define platform_arg %{!?platform:PLATFORM_CASSINI_HW}%{?platform:%platform}=1

%{!?dkms_source_tree:%define dkms_source_tree /usr/src}

%if 0%{?rhel}
%define distro_kernel_package_name kmod-%{name}
%else
%define distro_kernel_package_name %{name}-kmp
%endif

# Exclude -preempt kernel flavor, this seems to get built alongside the -default
# flavor for stock SLES. It doesn't get used, and its presence can cause issues
# (see NETCASSINI-4032)
%define kmp_args_common -x preempt -p %{_sourcedir}/%{name}.rpm_preamble

%if 0%{?rhel}
# On RHEL, override the kmod RPM name to include the kernel version it was built
# for; this allows us to package the same driver for multiple kernel versions.
%define kmp_args -n %name-k%%kernel_version %kmp_args_common
%else
%define kmp_args %kmp_args_common
%endif

# Used only for OBS builds; gets overwritten by actual Slingshot product version
%define release_extra 0

Name:           cray-slingshot-base-link
Version:        1.0.0
Release:        %(echo ${BUILD_METADATA})
Summary:        HPE Slingshot Base Link driver
License:        GPL-2.0
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  cray-cassini-headers-user
BuildRequires:  %kernel_module_package_buildreqs
Prefix:         /usr

# Generate a preamble that gets attached to the kmod RPM(s). Kernel module
# dependencies can be declared here. The 'Obsoletes' and 'Provides' lines for
# RHEL allow the package to be referred to by its base name without having to
# explicitly specify a kernel version.
%(/bin/echo -e "\
%if 0%%{?rhel} \n\
Obsoletes:      kmod-%%{name} \n\
Provides:       kmod-%%{name} = %%version-%%release \n\
%endif" > %{_sourcedir}/%{name}.rpm_preamble)

%if 0%{with shasta_premium}
# The nvidia-gpu-build-obs package (necessary for building against CUDA
# drivers) causes a bogus default kernel flavor to be added. This causes
# builds to fail, as upstream dependencies (i.e. SBL) are not built for
# default on shasta-premium. Work around this by explicitly excluding the
# default flavor on shasta-premium
%kernel_module_package -x 64kb default %kmp_args
%else
%kernel_module_package %kmp_args
%endif

%description
Slingshot Base Link driver

%package devel
Summary:    Development files for Slingshot Base Link driver
Requires:   cray-cassini-headers-user

%description devel
Development files for Slingshot Base Link driver

%package dkms
Summary:        DKMS support for %{name} kernel modules
Requires:       dkms
Requires:       cray-cassini-headers-user
Conflicts:      %{distro_kernel_package_name}
BuildArch:      noarch

%description dkms
DKMS support for %{name} kernel modules

%prep
%setup

set -- *
mkdir source
mv "$@" source/
mkdir obj

%build
for flavor in %flavors_to_build; do
    rm -rf obj/$flavor
    mkdir -p obj/$flavor
    cp -r source/* obj/$flavor/
    dir_save=$PWD
    cd obj/$flavor
    # QUIRK: the SBL makefile appends to KCPPFLAGS internally. By introducing
    # our own KCPPFLAGS as an environment variable, we prepend that value to
    # the makefile's KCPPFLAGS, rather than overriding it entirely
    KCPPFLAGS=' -I%{_includedir} ' make KDIR=%{kernel_source $flavor} SBL_EXTERNAL_BUILD=1 %platform_arg %{?_smp_mflags}
    cd $dir_save
done

%install
export INSTALL_MOD_PATH=$RPM_BUILD_ROOT
export INSTALL_MOD_DIR=extra/%{name}

echo %flavors_to_build
for flavor in %flavors_to_build; do
    make -C %{kernel_source $flavor} modules_install M=$PWD/obj/$flavor
    install -D $PWD/obj/$flavor/Module.symvers $RPM_BUILD_ROOT/%{prefix}/src/slingshot-base-link/$flavor/Module.symvers
done

install -D -m 644 source/sbl.h $RPM_BUILD_ROOT/%{_includedir}/linux/sbl.h
install -D -m 644 source/sbl_an.h $RPM_BUILD_ROOT/%{_includedir}/linux/sbl_an.h
install -D -m 644 source/sbl_kconfig.h $RPM_BUILD_ROOT/%{_includedir}/linux/sbl_kconfig.h
install -D -m 644 source/sbl_serdes_map.h $RPM_BUILD_ROOT/%{_includedir}/linux/sbl_serdes_map.h
install -D -m 644 source/uapi/sbl.h $RPM_BUILD_ROOT/%{_includedir}/uapi/sbl.h
install -D -m 644 source/uapi/sbl_cassini.h $RPM_BUILD_ROOT/%{_includedir}/uapi/sbl_cassini.h
install -D -m 644 source/uapi/sbl_counters.h $RPM_BUILD_ROOT/%{_includedir}/uapi/sbl_counters.h
install -D -m 644 source/uapi/sbl_iface_constants.h $RPM_BUILD_ROOT/%{_includedir}/uapi/sbl_iface_constants.h
install -D -m 644 source/uapi/sbl_serdes.h $RPM_BUILD_ROOT/%{_includedir}/uapi/sbl_serdes.h
install -D -m 644 source/uapi/sbl_serdes_defaults.h $RPM_BUILD_ROOT/%{_includedir}/uapi/sbl_serdes_defaults.h

%if 0%{?rhel}
# Centos/Rocky/RHEL does not exclude the depmod-generated modules.* files from
# the RPM, causing file conflicts when updating
find $RPM_BUILD_ROOT -iname 'modules.*' -exec rm {} \;
%endif

#
# install section for dkms
#
dkms_source_dir=%{dkms_source_tree}/%{name}-%{version}-%{release}
mkdir -p %{buildroot}/${dkms_source_dir}
cp -r source/* %{buildroot}/${dkms_source_dir}

rm -rf %{buildroot}/${dkms_source_dir}/Jenkinsfile.debian
rm -rf %{buildroot}/${dkms_source_dir}/cassini_rpm_build/Jenkinsfile.cassini
rm -rf %{buildroot}/${dkms_source_dir}/cassini_rpm_build/Jenkinsfile.cassini-basekernel
rm -rf %{buildroot}/${dkms_source_dir}/cassini_rpm_build/rpm_build.sh
rm -rf %{buildroot}/${dkms_source_dir}/cassini_rpm_build/runBuildPrep.basekernel.sh
rm -rf %{buildroot}/${dkms_source_dir}/cassini_rpm_build/set_slingshot_version.sh

echo "%dir ${dkms_source_dir}" > dkms-files
echo "${dkms_source_dir}" >> dkms-files

sed\
  -e '/^$/d'\
  -e '/^#/d'\
  -e 's/@PACKAGE_NAME@/%{name}/g'\
  -e 's/@PACKAGE_VERSION@/%{version}-%{release}/g'\
\
  %{buildroot}${dkms_source_dir}/dkms.conf.in > %{buildroot}${dkms_source_dir}/dkms.conf
rm -f %{buildroot}${dkms_source_dir}/dkms.conf.in

%files devel
%{_includedir}/linux/*.h
%{_includedir}/uapi/*.h
%{prefix}/src/slingshot-base-link/*/Module.symvers

%files dkms -f dkms-files

%changelog

%pre dkms

%post dkms
if [ -f /usr/libexec/dkms/common.postinst ] && [ -x /usr/libexec/dkms/common.postinst ]
then
    postinst=/usr/libexec/dkms/common.postinst
elif [ -f /usr/lib/dkms/common.postinst ] && [ -x /usr/lib/dkms/common.postinst ]
then
    postinst=/usr/lib/dkms/common.postinst
else
    echo "ERROR: did not find DKMS common.postinst"
    exit 1
fi
${postinst} %{name} %{version}-%{release}

%preun dkms
#
# `dkms remove` may return an error but that should stop the package from
# being removed.   The " || true" ensures this command line always returns
# success.   One reason `dkms remove` may fail is if someone (an admin)
# already manually removed this dkms package.  But there are some other
# "soft errors" (supposedly) that should not prevent the dkms package
# from being removed.
#
/usr/sbin/dkms remove -m %{name} -v %{version}-%{release} --all --rpm_safe_upgrade || true
