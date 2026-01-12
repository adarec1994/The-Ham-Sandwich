var viewController = {
    activeView: null,
    extensionMap: { },

    switchTo: function(newView, data) {
        if(newView != this.activeView && this.activeView != null && this.activeView.hide != null) {
            this.activeView.hide();
        }

        if(newView != null) {
            if(data != null)
                newView.show(data);
            else
                newView.show();
        }
        this.activeView = newView;
    },

    onFilePress: function(ext, data) {
        var dlg = this.extensionMap[ext];
        if(dlg != null) {
            try {
                this.switchTo(dlg, data);
            } catch (e) {
                console.log(e + "\n" + e.stack);
            }
        }
    }
};