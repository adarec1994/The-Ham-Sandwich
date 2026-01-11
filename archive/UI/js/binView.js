var binView = {
    show: function(fileName) {
        var scheme = API.filesystem.openBinFile(fileName);
        var info = JSON.parse(scheme);

        var entryModel = Backbone.Model.extend({ });

        var entries = Backbone.PageableCollection.extend({
            model: entryModel,
            url: "asset://local/.filesys/bin.json",
            state: {
                pageSize: 27,
                totalRecords: info.numEntries
            }
        });

        var _coll = new entries();

        var grid = new Backgrid.Grid({
            columns: [ { name: 'id', label: 'ID', editable: false, cell: Backgrid.IntegerCell.extend({ orderSeparator: '' }) }, { name: 'text', label: 'LocalizedText', editable: false, cell: 'string' } ],
            collection: _coll
        });

        var paginator = new Backgrid.Extension.Paginator({
            columns: [ { name: 'id', label: 'ID' }, { name: 'text', label: 'LocalizedText' } ],
            collection: grid.collection
        });

        $("#binEditorContainer").html("");
        $("#binEditorContainer").append(grid.render().$el);
        $("#binEditorPager").html("");
        $("#binEditorPager").append(paginator.render().$el);
        $('#binGlView').css({ 'display': 'block' });

        grid.collection.fetch();
    },

    hide: function() {
        $('#binGlView').css({ 'display': 'none' });
    }
};

viewController.extensionMap.bin = binView;