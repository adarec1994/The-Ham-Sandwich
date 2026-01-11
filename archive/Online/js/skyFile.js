var SkyFile = function (path, callback) {
    this.eoh = 0x470; // end of header, all offsets are relative to this
    this.callback = callback;
    this.file = new BinFile(path);
    this.file.onload = (function (self) {
        return function () {
            self.fileOpened();
        };
    })(this);

    this.file.open();
}

SkyFile.prototype.fileOpened = function () {
    this.header = this.file.read(SkyBaseHeader);

    var skyHeaders = [];

    this.skies = [];
    for (var i = 0; i < 4; ++i) {
        var skyHeader = this.file.read(SkyLineHeader);
        skyHeaders.push(skyHeader);
    }

    for (var i = 0; i < 4; ++i) {
        var header = skyHeaders[i];
        if (header.nValues2 == 0) {
            this.skies.push(null);
            continue;
        }

        this.file.seek(this.eoh + header.ofsTimes2);
        var sky = {
            times: [],
            colors: []
        };

        for (var j = 0; j < header.nValues2; ++j) {
            var time = this.file.readUint32();
            sky.times.push(time);
        }

        this.file.seek(this.eoh + header.ofsFrames2);
        for (var j = 0; j < header.nValues2; ++j) {
            var colorEntries = [];
            this.file.readUint64();
            this.file.readUint64();

            for (var k = 0; k < 17; ++k) {
                colorEntries.push(this.file.read(['r', 'f', 'g', 'f', 'b', 'f', 'a', 'f']));
            }

            sky.colors.push(colorEntries);
        }

        this.skies.push(sky);
    }

    this.callback(this);
}

var SkyLineHeader = [
    'number_unk', 'u64',
    'nUnk1', 'u64', 'ofsUnk1', 'u64',
    'nUnk2', 'u64', 'ofsUnk2', 'u64',
    'nUnk3', 'u64', 'ofsUnk3', 'u64',
    'nUnk4', 'u64', 'ofsUnk4', 'u64',
    'nUnk5', 'u64', 'ofsUnk5', 'u64',
    'nValues1', 'u64', 'ofsTimes1', 'u64', 'ofsFrames1', 'u64',
    'nValues2', 'u64', 'ofsTimes2', 'u64', 'ofsFrames2', 'u64'
];

var SkyBaseHeader = [
    'magic', 'u32',
    'version', 'u32',
    'unk1', 'u32',
    'unk2', 'u32',
    'maybeFloat', 'f',
    'unk3', 'u32',
    'lenXmlName', 'u64',
    'ofsXmlName', 'u64'
];