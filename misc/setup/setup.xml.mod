<?xml version="1.0" standalone="yes"?>
<!-- turtlearena is the name of the base product -->
<install product="turtlearena"
    desc="Turtle Arena"
    component="Foo Mod"
    version="1.1"
    >

    <option install="true">
	Foo
	<!--
	install symlink 'foo' into $PATH, pointing to a script
	called startfoo that gets installed into Turtle Arena's
	directory
	The script could look like this:
	#!/bin/sh
	exec turtlearena +set fs_game foo "$@"
    	exit 1
	-->
	<binary arch="any" libc="any"
	    symlink="foo"
	    binpath="startfoo">
	    startfoo
	</binary>
	<!--
	extract archive in Turtle Arena's directory.
	the archive must contain a subdirectory of course
	-->
	<files>
	    foo-1.1.zip
	</files>
    </option>
</install>
