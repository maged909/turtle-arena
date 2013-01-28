#!/bin/sh
set -e
if ! test -e "$SETUP_INSTALLPATH"/turtlearena.desktop.in; then
  exit 0
fi
xdg_desktop_menu=`which xdg-desktop-menu 2>/dev/null`
if test "x$xdg_desktop_menu" = x; then
  xdg_desktop_menu=./xdg-desktop-menu
fi
sed -e "s#^Exec=.*#Exec=$SETUP_INSTALLPATH/turtlearena#" \
    -e "s#^Icon=.*#Icon=$SETUP_INSTALLPATH/turtlearena.png#" \
    < $SETUP_INSTALLPATH/turtlearena.desktop.in \
    > $SETUP_INSTALLPATH/turtlearena.desktop
$xdg_desktop_menu install --novendor $SETUP_INSTALLPATH/turtlearena.desktop
