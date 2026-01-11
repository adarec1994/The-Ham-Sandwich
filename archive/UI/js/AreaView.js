var areaView = {
    show: function (fileName) {
        API.filesystem.loadArea(fileName);
        $('#areaGlView').css({ 'display': 'inline-block' });
    },

    hide: function () {
        $('#areaGlView').css({ 'display': 'none' });
    }
}

$(document).ready(function () {
    $('#areaObjExportButton').click(function () {
        API.area.exportObj();
    });

    $('#areaGlView .toolBarEntry').tooltip({
        position: {
            my: "center top+15",
            at: "center bottom",
            using: function (position, feedback) {
                $(this).css(position);
                $("<div>")
                    .addClass("arrow")
                    .addClass(feedback.vertical)
                    .addClass(feedback.horizontal)
                    .appendTo(this);
            }
        }
    });
});

viewController.extensionMap.area = areaView;