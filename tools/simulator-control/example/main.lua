display.setStatusBar( display.HiddenStatusBar )
display.setDefault( "background", 0.06, 0.08, 0.12 )

automationExample =
{
	started = false,
	tapCount = 0,
}
print( "AUTOMATION_EXAMPLE_READY" )

local title = display.newText(
{
	text = "Simulator control automation",
	x = display.contentCenterX,
	y = display.contentCenterY - 100,
	font = native.systemFontBold,
	fontSize = 24,
} )
title.automationId = "screenTitle"

local status = display.newText(
{
	text = "Waiting for input",
	x = display.contentCenterX,
	y = display.contentCenterY + 90,
	font = native.systemFont,
	fontSize = 18,
} )
status.automationId = "statusLabel"

timer.performWithDelay( 250, function()
	local playButton = display.newRoundedRect(
		display.contentCenterX,
		display.contentCenterY,
		220,
		72,
		16 )
	playButton:setFillColor( 0.1, 0.55, 0.95 )
	playButton.automationId = "playButton"

	local label = display.newText(
	{
		text = "Play",
		x = playButton.x,
		y = playButton.y,
		font = native.systemFontBold,
		fontSize = 28,
	} )
	label:setFillColor( 1 )
	label.isHitTestable = false
	label.automationId = "playButtonLabel"

	playButton:addEventListener( "tap", function()
		automationExample.started = true
		automationExample.tapCount = automationExample.tapCount + 1
		status.text = "Playing"
		print( "PLAY_BUTTON_TAPPED", automationExample.tapCount )
		return true
	end )
end )
