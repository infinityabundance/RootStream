Name:           rootstream
Version:        0.1.0
Release:        1%{?dist}
Summary:        Native Linux game streaming — direct kernel access, no compositor overhead

License:        MIT
URL:            https://github.com/yourusername/rootstream
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  libdrm-devel
BuildRequires:  libva-devel

Requires:       libdrm
Requires:       libva

%description
RootStream is a native Linux game-streaming application that uses direct DRM
kernel access and VA-API hardware encoding to stream with minimal latency and
no compositor overhead.

%prep
%autosetup

%build
%make_build

%install
install -Dpm 0755 rootstream     %{buildroot}%{_bindir}/rootstream
install -Dpm 0644 rootstream.service \
    %{buildroot}%{_unitdir}/rootstream@.service
install -Dpm 0644 README.md      %{buildroot}%{_docdir}/%{name}/README.md
install -Dpm 0644 LICENSE        %{buildroot}%{_licensedir}/%{name}/LICENSE

%post
%systemd_post rootstream@.service

%preun
%systemd_preun rootstream@.service

%postun
%systemd_postun_with_restart rootstream@.service

%files
%license LICENSE
%doc README.md
%{_bindir}/rootstream
%{_unitdir}/rootstream@.service

%changelog
* Thu Jan 01 2026 RootStream Team <rootstream@example.com> - 0.1.0-1
- Initial RPM packaging
