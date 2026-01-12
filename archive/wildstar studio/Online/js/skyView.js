var skyView = {
    show: function (file) {
        $('#glDrawArea').css({ 'display': 'none' });
        $('#skyGlView').empty();

        var skyFile = new SkyFile(file, function (file) {
            for (var i in file.skies) {
                var html = '<div><p style="color: white">Sky - ' + i + '</p>';
                if (file.skies[i] == null) {
                    html += '<p style="color: white">No unk7 data</p>';
                } else {
                    var values = {
                        0: [],
                        1: [],
                        2: [],
                        3: [],
                        4: [],
                        5: [],
                        6: [],
                        7: [],
                        8: [],
                        9: [],
                        10: [],
                        11: [],
                        12: [],
                        13: [],
                        14: [],
                        15: [],
                        16: []
                    };
                    for (var j = 0; j < file.skies[i].times.length; ++j) {
                        for (var k = 0; k < 17; ++k) {
                            values[k].push({ time: file.skies[i].times[j], value: file.skies[i].colors[j][k] });
                        }
                    }
                    for (var k = 0; k < 17; ++k) {
                        var gradient = '-webkit-linear-gradient(top left, ';
                        for (var j = 0; j < file.skies[i].times.length; ++j) {
                            if (j != 0) {
                                gradient += ', ';
                            }

                            var color = values[k][j];
                            var r = Math.floor(color.value.r * 255).toString(16);
                            var g = Math.floor(color.value.g * 255).toString(16);
                            var b = Math.floor(color.value.b * 255).toString(16);

                            if (r.length < 2) {
                                r = '0' + r;
                            }

                            if (g.length < 2) {
                                g = '0' + g;
                            }

                            if (b.length < 2) {
                                b = '0' + b;
                            }

                            gradient += '#' + r + g + b + ' ' + Math.floor(file.skies[i].times[j] / 864) + '%';
                        }

                        gradient += ')';

                        html += '<div style="width: 150px; height: 150px; background-image: ' + gradient + '; display: inline-block; margin-left: 5px;"></div>'
                    }
                }
                $('#skyGlView').append($(html));
            }

            $('#skyGlView').css({ 'display': 'block' });
        });
    },

    hide: function () {
        $('#skyGlView').css({ 'display': 'none' });
    }
};

viewController.extensionMap.sky = skyView;