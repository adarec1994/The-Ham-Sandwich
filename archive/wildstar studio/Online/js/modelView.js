var modelView = {
    show: function(file) {
        this.model = new M3Model(file);
        $('#modelGlView').css({ 'display': 'inline-block' });
    },

    hide: function() {
        $('#modelGlView').css({ 'display': 'none' });
    },

    isRotating: true
};

viewController.extensionMap.m3 = modelView;

$('document').ready(function() {
    $('#modelGlView .toolBarEntry').tooltip({
        position: {
            my: "center top+15",
            at: "center bottom",
            using: function(position, feedback) {
                $(this).css(position);
                $("<div>")
                    .addClass("arrow" )
                    .addClass(feedback.vertical)
                    .addClass(feedback.horizontal)
                    .appendTo(this);
            }
        }
    });

    $('#objExportButton').click(function() {
        API.model.exportObj();
    });

    $('#toggleRotationButton').click(function() {
        if(modelView.isRotating === false) {
            API.model.restartRotation();
            $('#toggleRotationButton img').attr('src', 'asset://local/images/pause-512.png');
        } else {
            API.model.pauseRotation();
            $('#toggleRotationButton img').attr('src', 'asset://local/images/play-512.png');
        }

        modelView.isRotating = !modelView.isRotating;
    });

    $('#resetRotationButton').click(function() {
        API.model.resetRotation();
    });

    $('#toggleViewButton').click(function() {
        API.model.toggleViewMode();
    });

    $('#toggleNormalsButton').click(function() {
        API.model.toggleNormals();
    });
});