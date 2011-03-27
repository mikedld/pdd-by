# Copyright 1999-2011 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=2
inherit cmake-utils eutils mercurial

DESCRIPTION="Road rules frontend for Belarus"
HOMEPAGE="http://code.google.com/p/pdd-by/"
EHG_REPO_URI="http://pdd-by.googlecode.com/hg"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~x86 ~amd64"
IUSE="aqua gtk qt4 static-libs"

RDEPEND="
	dev-db/sqlite:3
	dev-libs/libpcre:3[unicode]
	!aqua? ( virtual/libiconv )
	gtk? ( x11-libs/gtk+ )
	qt4? (
		x11-libs/qt-core:4
		x11-libs/qt-gui:4
	)
"
DEPEND="
	dev-util/cmake
	${RDEPEND}
"

S="${WORKDIR}/hg"

src_configure() {
	mycmakeargs=(
		"$(cmake-utils_use aqua PDDBY_FRONTEND_BUILD_COCOA)"
		"$(cmake-utils_use gtk PDDBY_FRONTEND_BUILD_GTK)"
		"$(cmake-utils_use qt4 PDDBY_FRONTEND_BUILD_QT)"
		"$(cmake-utils_use static-libs PDDBY_STATIC_LIBS)"
	)
	cmake-utils_src_configure
}
