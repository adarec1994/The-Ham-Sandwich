var i3View = {
    show: function(file) {
        var modelObj = { };
        var infoJSON = API.filesystem.loadI3(file, modelObj);
        if(!infoJSON) {
            return;
        }

        modelObj = JSON.parse(infoJSON);
        var htmlStr = "";

        for(var i in modelObj.sectors) {
            var sec = modelObj.sectors[i];
            htmlStr += '<input type="radio" name="sector_choose" id="sector_' + i + '" number="' + i + '" ';
            if(i == 0) {
                htmlStr += 'checked';
            }

            htmlStr += '><label for="sector_' + i + '">' + sec.name + '</label><br />';
        }

        $('#sectorRadioButtons').html(htmlStr);

        $('#i3ModelView').css({ 'display': 'block' });

        $('#sectorRadioButtons input').click(function() {
            var num = $(this).attr('number');
            API.model.setI3Sector(num);
        });
    },

    hide: function() {
        $('#i3ModelView').css({ 'display': 'none' });
    }
}

viewController.extensionMap.i3 = i3View;