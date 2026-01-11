var tableView = {
    show: function(fileName) {
        var scheme = API.filesystem.openTable(fileName);
        var info = JSON.parse(scheme);

        var entryModel = Backbone.Model.extend({ });

        var entries = Backbone.PageableCollection.extend({
            model: entryModel,
            url: "asset://local/.filesys/tbl.json",
            state: {
                pageSize: 27,
                totalRecords: info.numEntries
            }
        });

        var _coll = new entries();

        var grid = new Backgrid.Grid({
            columns: info.columns,
            collection: _coll
        });

        var paginator = new Backgrid.Extension.Paginator({
            columns: info.columns,
            collection: grid.collection
        });

        $("#tableEditorContainer").html("");
        $("#tableEditorContainer").append(grid.render().$el);
        $("#tableEditorPager").html("");
        $("#tableEditorPager").append(paginator.render().$el);
        $('#tableGlView').css({ 'display': 'block' });

        grid.collection.fetch();
    },

    hide: function() {
        $('#tableGlView').css({ 'display': 'none' });
    }
};

viewController.extensionMap.tbl = tableView;