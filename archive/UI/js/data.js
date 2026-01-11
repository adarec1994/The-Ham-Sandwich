var dataViewGlobal = {
    curRot: 0,
    rotInterval: 0
};

var onDataFolderChanged = function(newFolder) {
    $('#dataFolderPath').text("Current Path: " + newFolder);
    $('#dataFilePath').html("<span class='descLabel'>Current file path:</span><br />" + newFolder);
    $('#nonPathSelectedView').css({ 'display': 'none' });
    $('#fileToolbarView').css({ 'display': 'block' });

    dataViewGlobal.rotInterval = setInterval(function() {
        dataViewGlobal.curRot += 6;
        $('#pathLoadRotate').css({ 'transform': 'rotate(' + dataViewGlobal.curRot + 'deg)' });
    }, 16);

    $('#setDataFolderEntry').css({ 'display': 'none' });
    $('#underConstruction').css({ 'display': 'inline-block' });
}

var onUpdateFileLoadProgress = function(cur, total) {
    $('#fileLoadProgress').progressbar("value", Math.min(100, (cur / total) * 100.0));
    $('#fileLoadIndicator').text(cur + " / " + total);
}

var onFileLoadDone = function(rootNodes) {
    $('#fileToolbarView_load').css({ 'display': 'none' });
    $('#fileToolbarView_tree').css({ 'display': 'block' });
    $('#toolBarExtractButton').removeClass('hidden');
    $('#toolBarExtractFilter').removeClass('hidden');
    $('#toolBarAudio').removeClass('hidden');

    clearInterval(dataViewGlobal.rotInterval);

    $('#tree').dynatree({
        debugLevel: 0,
        persist: true,
        children: rootNodes,
        fx: { height: "toggle", duration: 200 },
        onLazyRead: function(node) {
            node.appendAjax({
                url: "asset://local/.filesys/get_child.json",
                data: {
                    key: node.data.key
                }
            });
        },

        onFocus: function(node) {
            if(node.data.isFolder) {
                return;
            }

            var extension = API.filesystem.getExtension(node.data.key);
            viewController.onFilePress(extension, node.data.key);
        }
    });
}

var onExtractFile = function() {
    var tree = $("#tree").dynatree("getTree");
    var node = tree.getActiveNode();
    if(node == null) {
        $('#errorText').text("No node was selected from the file view. Cannot extract from no node :(");
        $('#errorDialog').dialog('open');
        return;
    }

    var entry = node.data.key;
    API.filesystem.extractEntry(entry);
}

var onAsyncExtractionOperation = function() {
    $('#asyncLoadProgress').progressbar({ value: false });
    $('#asyncDialog').dialog('open');
};

var onAsyncExtractionDone = function() {
    $('#asyncDialog').dialog('close');
};

function onExtractFiltered() {
    var subElems = $('#extensionList').children();
    var regex = $('#regexInput').val();

    if (subElems.length == 0 && regex.length == 0) {
        return;
    }

    $('#asyncLoadProgress').progressbar({ value: false });
    $('#asyncDialog').dialog('open');

    var extMap = [];
    subElems.each(function () {
        extMap.push($(this).find('span:first-child').text());
    });

    var filterParams = {
        extensions: extMap,
        regex: regex,
        areaAsObj: $('#filteredExportAreaObj').prop('checked'),
        texAsBmp: $('#filteredExportTexAsBmp').prop('checked'),
        m3AsObj: $('#filteredExportM3Obj').prop('checked'),
        tblAsCsv: $('#filteredExportTblCsv').prop('checked'),
        entry: $('#basePathInput').val()
    };

    API.filesystem.filteredExport(filterParams);
}

$('document').ready(function() {
    $('#setDataFolderEntry').click(function() {
        API.filesystem.setPath();
    });

    $('#selectDataFolderToolEntry').click(function() {
        $('#setDataFolderEntry').click();
    });

    $('#toolBarExtractButton').click(function() {
        onExtractFile();
    });

    $('#toolBarExtractButton').tooltip({
        position: {
            my: "center bottom-15",
            at: "center top",
            using: function(position, feedback) {
                $(this).css(position);
                $("<div>")
                    .addClass( "arrow" )
                    .addClass( feedback.vertical )
                    .addClass( feedback.horizontal )
                    .appendTo( this );
            }
        }
    });


    $('#fileLoadProgress').progressbar({
        value: 30
    });
});