@echo off
echo Setting environment for minimal Visual C++ 6
set INCLUDE=%MSVCDir%\VC98\Include
set LIB=%MSVCDir%\VC98\Lib
set PATH=%MSVCDir%\VC98\Bin;%MSVCDir%\Common\MSDev98\Bin\;%PATH%

echo -- Compiler is MSVC6

set XASH3DSRC=..\..\Xash3D_original
set INCLUDES=-I../common -I../engine -I../pm_shared -I../game_shared -I../public
set SOURCES=agrunt.cpp ^
	airtank.cpp ^
	aflock.cpp ^
	ammunition.cpp ^
	animating.cpp ^
	animation.cpp ^
	apache.cpp ^
	barnacle.cpp ^
	barney.cpp ^
	bigmomma.cpp ^
	bloater.cpp ^
	bmodels.cpp ^
	bullsquid.cpp ^
	buttons.cpp ^
	cbase.cpp ^
	client.cpp ^
	combat.cpp ^
	controller.cpp ^
	crossbow.cpp ^
	crowbar.cpp ^
	defaultai.cpp ^
	displacer.cpp ^
	doors.cpp ^
	drillsergeant.cpp ^
	eagle.cpp ^
	effects.cpp ^
	egon.cpp ^
	explode.cpp ^
	fgrunt.cpp ^
	flyingmonster.cpp ^
	followingmonster.cpp ^
	func_break.cpp ^
	func_tank.cpp ^
	game.cpp ^
	gamerules.cpp ^
	gargantua.cpp ^
	gauss.cpp ^
	genericmonster.cpp ^
	geneworm.cpp ^
	ggrenade.cpp ^
	globals.cpp ^
	glock.cpp ^
	gman.cpp ^
	gonome.cpp ^
	grapple.cpp ^
	h_ai.cpp ^
	h_battery.cpp ^
	h_cine.cpp ^
	h_cycler.cpp ^
	h_export.cpp ^
	handgrenade.cpp ^
	hassassin.cpp ^
	headcrab.cpp ^
	healthkit.cpp ^
	hgrunt.cpp ^
	hwgrunt.cpp ^
	hornet.cpp ^
	hornetgun.cpp ^
	houndeye.cpp ^
	ichthyosaur.cpp ^
	islave.cpp ^
	items.cpp ^
	knife.cpp ^
	leech.cpp ^
	lights.cpp ^
	locus.cpp ^
	m249.cpp ^
	maprules.cpp ^
	massn.cpp ^
	medkit.cpp ^
	monstermaker.cpp ^
	monsters.cpp ^
	monsterstate.cpp ^
	mortar.cpp ^
	mp5.cpp ^
	multiplay_gamerules.cpp ^
	nihilanth.cpp ^
	nodes.cpp ^
	nuclearbomb.cpp ^
	observer.cpp ^
	op4mortar.cpp ^
	osprey.cpp ^
	pathcorner.cpp ^
	pipewrench.cpp ^
	pitdrone.cpp ^
	pitworm.cpp ^
	plane.cpp ^
	plats.cpp ^
	player.cpp ^
	playermonster.cpp ^
	python.cpp ^
	rat.cpp ^
	rgrunt.cpp ^
	recruit.cpp ^
	roach.cpp ^
	ropes.cpp ^
	rpg.cpp ^
	satchel.cpp ^
	schedule.cpp ^
	scientist.cpp ^
	scripted.cpp ^
	shockbeam.cpp ^
	shockrifle.cpp ^
	shocktrooper.cpp ^
	shotgun.cpp ^
	singleplay_gamerules.cpp ^
	skill.cpp ^
	sniperrifle.cpp ^
	sound.cpp ^
	soundent.cpp ^
	spectator.cpp ^
	spore.cpp ^
	sporelauncher.cpp ^
	squadmonster.cpp ^
	squeakgrenade.cpp ^
	subs.cpp ^
	talkmonster.cpp ^
	teamplay_gamerules.cpp ^
	tempmonster.cpp ^
	tentacle.cpp ^
	triggers.cpp ^
	tripmine.cpp ^
	turret.cpp ^
	util.cpp ^
	voltigore.cpp ^
	weapons.cpp ^
	world.cpp ^
	xen.cpp ^
	zombie.cpp ^
	../pm_shared/pm_debug.c ../pm_shared/pm_math.c ../pm_shared/pm_shared.c ../game_shared/tex_materials.c
set DEFINES=/DCLIENT_WEAPONS /Dsnprintf=_snprintf /DNO_VOICEGAMEMGR
set LIBS=user32.lib
set OUTNAME=hl.dll
set DEBUG=/debug

cl %DEFINES% %LIBS% %SOURCES% %INCLUDES% -o %OUTNAME% /link /dll /out:%OUTNAME% %DEBUG% /def:".\hl.def"

echo -- Compile done. Cleaning...

del *.obj *.exp *.lib *.ilk
echo -- Done.
