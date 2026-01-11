var settingsDialog = {
    show: function() {
        $('#settingsDialog').dialog('open');
        $('#fileSaveFolderSelect').click(function() {
            var path = API.filesystem.getFolder();
            if(path.length != null && path.length > 0) {
                $('#fileSaveFolderInput').val(path);
            }
        });
    }
};

$('document').ready(function() {
    $('#settingsDialog').dialog({
        width: 550,
        height: 380,
        modal: true,
        autoOpen: false,
        resizable: false,
        closeOnEscape: true,
        buttons: {
            "Save Settings": function() {
                API.filesystem.setExtractionFolder($('#fileSaveFolderInput').val());
                API.filesystem.setTextureAsBmp($('#exportBmpCb').prop('checked'));
                API.filesystem.setTblAsCsv($('#exportCsvCb').prop('checked'));
                API.filesystem.setMouseSensitivity($('#mouseSensitivity').val());
                $(this).dialog('close');
            },

            "Cancel": function() { $(this).dialog('close'); }
        }
    });

    $('#toolBarSettingsButton').click(settingsDialog.show);
    $('#fileSaveFolderInput').val(API.filesystem.getExtractionFolder());
    if(API.filesystem.textureAsBmp()) {
        $('#exportBmpCb').prop('checked', 'true');
    }

    if(API.filesystem.tblAsCsv()) {
        $('#exportCsvCb').prop('checked', 'true');
    }

    $('#aboutButton').click(function () {
        $('#aboutDialog').dialog('open');
    });

    $('#debugBarEntry').click(function () {
        //$('#setDataFolderEntry').css({ 'display': 'block' });
        //location.reload(true);
    });

    $('#errorDialog').dialog({
        width: 450,
        height: 300,
        modal: true,
        autoOpen: false,
        resizable: false,
        closeOnEscape: true,
        buttons: {
            "Ok": function() {
                $(this).dialog('close');
            }
        }
    });

    $('#asyncDialog').dialog({
        width: 350,
        height: 200,
        modal: true,
        autoOpen: false,
        resizable: false,
        closeOnEscape: false,
        dialogClass: "no-close"
    });

    $('#aboutDialog').dialog({
        width: 450,
        height: 400,
        modal: true,
        autoOpen: false,
        resizable: false,
        closeOnEscape: true,
        buttons: {
            "Ok": function() { $(this).dialog('close'); }
        }
    });
});