$(document).ready(function() {
	$('#toolBarAudio').click(function() {
		$('#audioViewer').css({ display: 'block' });
	});

	$('#audioBackButton').click(function() {
		$('#audioViewer').css({ display: 'none' });
	});
});