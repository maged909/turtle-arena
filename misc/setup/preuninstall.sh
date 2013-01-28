#!/bin/sh
if test -e "$SETUP_INSTALLPATH"/turtlearena.desktop.in; then
  xdg_desktop_menu=`which xdg-desktop-menu 2>/dev/null`
  if test "x$xdg_desktop_menu" = x; then
    xdg_desktop_menu=./xdg-desktop-menu
  fi
  $xdg_desktop_menu uninstall --novendor turtlearena.desktop
  rm turtlearena.desktop
fi
