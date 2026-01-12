var Texture = function (path) {
    this.id = Texture.defTexture;
    this.file = new BinFile(path);
    this.file.onload = (function (self) {
        return function () {
            self.fileOpened();
        };
    })(this);
}

Texture.prototype.load = function () {
    this.file.open();
}

Texture.prototype.bind = function () {
    gl.bindTexture(gl.TEXTURE_2D, this.id);
}

Texture.prototype.unbind = function () {
    gl.bindTexture(gl.TEXTURE_2D, null);
}

Texture.prototype.fileOpened = function() {
    this.header = this.file.read(Texture.texHeader);
    if(this.header.texFormatIndex >= Texture.formatEntries.length) {
        return;
    }

    var fmt = Texture.formatEntries[this.header.texFormatIndex];
    if (fmt[10] < 0) {
        throw new TypeError('Texture format not supported');
    }

    var compressed = fmt[12];
    var glFormat = 0;
    var blockSize = fmt[9];

    switch (fmt[10]) {
        case 1:
            glFormat = Texture.RGB_S3TC_DXT1_EXT;
            break;

        case 2:
            glFormat = Texture.RGBA_S3TC_DXT3_EXT;
            break;

        case 3:
            glFormat = Texture.RGBA_S3TC_DXT5_EXT;
            break;
    }

    this.id = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, this.id);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);

    var isnpot = false;
    if ((this.header.width & (this.header.width - 1)) != 0 || (this.header.height & (this.header.height - 1)) != 0) {
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        isnpot = true;
    } else {
        isnpot = false;
    }

    for (var i = this.header.mipCount - 1; i >= 0; --i) {
        var curw = this.header.width >> i;
        var curh = this.header.height >> i;
        curw = Math.max(1, curw);
        curh = Math.max(1, curh);

        var size = Math.floor((curw + 3) / 4) * Math.floor((curh + 3) / 4) * blockSize;
        if (compressed == false) {
            size = curw * curh * blockSize;
        }

        var data = this.file.readBytes(size);
        if (compressed == false) {
            gl.texImage2D(gl.TEXTURE_2D, i, gl.RGBA, curw, curh, 0, gl.RGBA, gl.UNSIGNED_BYTE, data);
        } else {
            gl.compressedTexImage2D(gl.TEXTURE_2D, i, glFormat, curw, curh, 0, data);
        }
    }

    if (isnpot == false) {
        if (1 << (this.header.mipCount - 1) < this.header.width || 1 << (this.header.mipCount - 1) < this.header.height) {
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        }
    }

    gl.bindTexture(gl.TEXTURE_2D, null);

    if (this.onload) {
        this.onload.apply(this);
    }
}

Texture.loadDXT5 = function(layer, buffer, width, height, file) {
    var numBlocksFull = Math.floor((width * height) / 16);
    var partial = ((width * height) % 16) != 0;

    var blockData = new Uint32Array((numBlocksFull + (partial ? 1 : 0)) * 16);

    for (var i = 0; i < numBlocksFull; ++i) {
        Texture.dxt5GetBlock(blockData, i * 16, file);
    }

    if (partial) {
        Texture.dxt5GetBlock(blockData, numBlocksFull * 16, file);
    }

    var colorData = new Uint32Array(buffer);
    for (var y = 0; y < height; ++y) {
        for (var x = 0; x < width; ++x) {
            var bx = Math.floor(x / 4);
            var by = Math.floor(y / 4);

            var ibx = x % 4;
            var iby = y % 4;

            var blockIndex = by * Math.floor((width / 4)) + bx;
            var innerIndex = iby * 4 + ibx;
            colorData[y * width + x] = blockData[blockIndex * 16 + innerIndex];
        }
    }

    return colorData;
}

Texture.dxt5GetBlock = function (blockData, start, file) {
    var alphaValues = [0, 0, 0, 0, 0, 0, 0, 0];
    var alphaLookup = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

    var alpha1 = file.readUint8();
    var alpha2 = file.readUint8();

    alphaValues[0] = alpha1;
    alphaValues[1] = alpha2;

    if (alpha1 > alpha2) {
        for (var i = 0; i < 6; ++i) {
            alphaValues[i + 2] = Math.floor(((6.0 - i) * alpha1 + (1.0 + i) * alpha2) / 7.0);
        }
    } else {
        for (var i = 0; i < 4; ++i) {
            alphaValues[i + 2] = Math.floor(((4.0 - i) * alpha1 + (1.0 + i) * alpha2) / 5.0);
            alphaValues[6] = 0;
            alphaValues[7] = 255;
        }
    }

    var lookLower = file.readUint32();
    var lookUpper = file.readUint16();
    for (var i = 0; i < 10; ++i) {
        alphaLookup[i] = (lookLower >> (i * 3)) & 7;
    }

    alphaLookup[10] = lookLower >> 30;
    alphaLookup[10] += (lookUpper & 1) * 3;

    for (var i = 0; i < 5; ++i) {
        alphaLookup[11 + i] = (lookUpper >> (2 + (i * 3))) & 7;
    }

    var color1 = file.readUint16();
    var color2 = file.readUint16();

    var rgb1 = Texture.rgb565ToRGB8(color1);
    var rgb2 = Texture.rgb565ToRGB8(color2);
    var rgb3 = [0, 0, 0];
    var rgb4 = [0, 0, 0];

    if (color1 > color2) {
        for (var i = 0; i < 3; ++i) {
            rgb4[i] = Math.floor((rgb1[i] + 2 * rgb2[i]) / 3);
            rgb3[i] = Math.floor((2 * rgb1[i] + rgb2[i]) / 3);
        }
    } else {
        for (var i = 0; i < 3; ++i) {
            rgb3[i] = Math.floor((rgb1[i] + rgb2[i]) / 2);
            rgb4[i] = 0;
        }
    }

    var indices = file.readUint32();
    var tableIndices = [];
    for (var i = 0; i < 16; ++i) {
        tableIndices.push((indices >> (2 * i)) & 3);
    }

    var colorArray = [
        rgb1[0] | rgb1[1] << 8 | rgb1[2] << 16 | 0xFF000000,
        rgb2[0] | rgb2[1] << 8 | rgb2[2] << 16 | 0xFF000000,
        rgb3[0] | rgb3[1] << 8 | rgb3[2] << 16 | 0xFF000000,
        rgb4[0] | rgb4[1] << 8 | rgb4[2] << 16 | 0xFF000000
    ];

    for (var index = 0; index < 16; ++index) {
        var color = colorArray[tableIndices[index]];
        var alpha = alphaValues[alphaLookup[index]];
        color &= 0x00FFFFFF;
        color |= (alpha << 24);
        blockData[start + index] = color;
    }
}

Texture.loadDXT3 = function (layer, buffer, width, height, file) {
    var numBlocksFull = Math.floor((width * height) / 16);
    var partial = ((width * height) % 16) != 0;

    var blockData = new Uint32Array((numBlocksFull + (partial ? 1 : 0)) * 16);

    for (var i = 0; i < numBlocksFull; ++i) {
        Texture.dxt3GetBlock(blockData, i * 16, file);
    }

    if (partial) {
        Texture.dxt3GetBlock(blockData, numBlocksFull * 16, file);
    }

    var colorData = new Uint32Array(buffer);
    for (var y = 0; y < height; ++y) {
        for (var x = 0; x < width; ++x) {
            var bx = Math.floor(x / 4);
            var by = Math.floor(y / 4);

            var ibx = x % 4;
            var iby = y % 4;

            var blockIndex = by * Math.floor((width / 4)) + bx;
            var innerIndex = iby * 4 + ibx;
            colorData[y * width + x] = blockData[blockIndex * 16 + innerIndex];
        }
    }

    return colorData;
}

Texture.dxt3GetBlock = function (blockData, start, file) {
    var alphalow = file.readUint32();
    var alphaHigh = file.readUint32();

    var alphaValues = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    for (var i = 0; i < 8; ++i) {
        alphaValues[i] = Math.floor(((alphaLow >> (4 * i)) & 0x0F) * 15);
        alphaValues[i + 8] = Math.floor(((alphaHigh >> (4 * i)) & 0x0F) * 15);
    }

    var color1 = file.readUint16();
    var color2 = file.readUint16();

    var rgb1 = Texture.rgb565ToRGB8(color1);
    var rgb2 = Texture.rgb565ToRGB8(color2);
    var rgb3 = [0, 0, 0];
    var rgb4 = [0, 0, 0];

    if (color1 > color2) {
        for (var i = 0; i < 3; ++i) {
            rgb4[i] = Math.floor((rgb1[i] + 2 * rgb2[i]) / 3);
            rgb3[i] = Math.floor((2 * rgb1[i] + rgb2[i]) / 3);
        }
    } else {
        for (var i = 0; i < 3; ++i) {
            rgb3[i] = Math.floor((rgb1[i] + rgb2[i]) / 2);
            rgb4[i] = 0;
        }
    }

    var indices = file.readUint32();
    var tableIndices = [];
    for (var i = 0; i < 16; ++i) {
        tableIndices.push((indices >> (2 * i)) & 3);
    }

    var colorArray = [
        rgb1[0] | rgb1[1] << 8 | rgb1[2] << 16 | 0xFF000000,
        rgb2[0] | rgb2[1] << 8 | rgb2[2] << 16 | 0xFF000000,
        rgb3[0] | rgb3[1] << 8 | rgb3[2] << 16 | 0xFF000000,
        rgb4[0] | rgb4[1] << 8 | rgb4[2] << 16 | 0xFF000000
    ];

    for (var index = 0; index < 16; ++index) {
        var color = colorArray[tableIndices[index]];
        var alpha = alphaValues[index];
        color &= 0x00FFFFFF;
        color |= (alpha << 24);
        blockData[start + index] = color;
    }
}

Texture.loadDXT1 = function (layer, buffer, width, height, file) {
    var numBlocksFull = Math.floor((width * height) / 16);
    var partial = ((width * height) % 16) != 0;

    var blockData = new Uint32Array((numBlocksFull + (partial ? 1 : 0)) * 16);

    for (var i = 0; i < numBlocksFull; ++i) {
        Texture.dxt1GetBlock(blockData, i * 16, file);
    }

    if (partial) {
        Texture.dxt1GetBlock(blockData, numBlocksFull * 16, file);
    }

    var colorData = new Uint32Array(buffer);
    for (var y = 0; y < height; ++y) {
        for (var x = 0; x < width; ++x) {
            var bx = Math.floor(x / 4);
            var by = Math.floor(y / 4);

            var ibx = x % 4;
            var iby = y % 4;

            var blockIndex = by * Math.floor((width / 4)) + bx;
            var innerIndex = iby * 4 + ibx;
            colorData[y * width + x] = blockData[blockIndex * 16 + innerIndex];
        }
    }

    return colorData;
}

Texture.dxt1GetBlock = function(blockData, start, file) {
    var color1 = file.readUint16();
    var color2 = file.readUint16();

    var rgb1 = Texture.rgb565ToRGB8(color1);
    var rgb2 = Texture.rgb565ToRGB8(color2);
    var rgb3 = [0, 0, 0];
    var rgb4 = [0, 0, 0];

    if (color1 > color2) {
        for (var i = 0; i < 3; ++i) {
            rgb4[i] = Math.floor((rgb1[i] + 2 * rgb2[i]) / 3);
            rgb3[i] = Math.floor((2 * rgb1[i] + rgb2[i]) / 3);
        }
    } else {
        for (var i = 0; i < 3; ++i) {
            rgb3[i] = Math.floor((rgb1[i] + rgb2[i]) / 2);
            rgb4[i] = 0;
        }
    }

    var indices = file.readUint32();
    var tableIndices = [];
    for (var i = 0; i < 16; ++i) {
        tableIndices.push((indices >> (2 * i)) & 3);
    }

    var colorArray = [
        rgb1[0] | rgb1[1] << 8 | rgb1[2] << 16 | 0xFF000000,
        rgb2[0] | rgb2[1] << 8 | rgb2[2] << 16 | 0xFF000000,
        rgb3[0] | rgb3[1] << 8 | rgb3[2] << 16 | 0xFF000000,
        rgb4[0] | rgb4[1] << 8 | rgb4[2] << 16 | 0xFF000000
    ]

    for (var index = 0; index < 16; ++index) {
        blockData[start + index] = colorArray[tableIndices[index]];
    }
}

Texture.rgb565ToRGB8 = function(input) {
    var r = (input & 0x1F);
    var g = (input >> 5) & 0x3F;
    var b = (input >> 11) & 0x1F;

    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);

    return [r, g, b];
}

Texture.texHeader = [
    'magic', 'u32',
    'version', 'u32',
    'width', 'u32',
    'height', 'u32',
    'depth', 'u32',
    'sides', 'u32',
    'mipCount', 'u32',
    'texFormatIndex', 'u32'
];

Texture.formatEntries = [
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, 0, 0, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, 0, 0, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 4, 3, 2, 4, 3, 2, 1, 0, 0, 8, 1, 1, true ],
    [ 4, 3, 2, 4, 3, 2, 1, 0, 0, 16, 2, 1, true ],
    [ 4, 3, 2, 4, 3, 2, 1, 0, 0, 16, 3, 1, true ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 8, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 16, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, -1, -1, false ],
    [ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, -1, -1, false ]
];

Texture.initDefaultTexture = function () {
    Texture.defTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, Texture.defTexture);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 1, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array([0x00, 0xFF, 0x00, 0xFF]));
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.bindTexture(gl.TEXTURE_2D, null);

    Texture.RGB_S3TC_DXT1_EXT = GxContext.S3TC.COMPRESSED_RGB_S3TC_DXT1_EXT;
    Texture.RGBA_S3TC_DXT3_EXT = GxContext.S3TC.COMPRESSED_RGBA_S3TC_DXT3_EXT;
    Texture.RGBA_S3TC_DXT5_EXT = GxContext.S3TC.COMPRESSED_RGBA_S3TC_DXT5_EXT;
}

Texture.skipLowLayers = function (file, header, blockSize) {
    var compressed = Texture.formatEntries[header.texFormatIndex];

    for (var i = header.mipCount - 1; i > 0; --i) {
        var curw = header.width >> i;
        var curh = header.height >> i;
        curw = Math.max(1, curw);
        curh = Math.max(1, curh);

        var size = Math.floor((curw + 3) / 4) * Math.floor((curh + 3) / 4) * blockSize;
        if (compressed == false) {
            size = curw * curh * 4;
        }

        file.seekMod(size);
    }
}

Texture.getBitmapMemory = function (path, callback) {
    var file = new BinFile(path);
    file.onload = (function (callback) {
        return function () {
            var header = this.read(Texture.texHeader);
            if (header.texFormatIndex >= Texture.formatEntries.length) {
                return;
            }

            var fmt = Texture.formatEntries[header.texFormatIndex];
            if (fmt[10] < 0) {
                throw new TypeError('Texture format not supported');
            }

            var compressed = fmt[12];
            var glFormat = 0;
            var blockSize = fmt[9];

            switch (fmt[10]) {
                case 1:
                    {
                        Texture.skipLowLayers(file, header, 8);
                        var buffer = new ArrayBuffer(header.width * header.height * 4);
                        Texture.loadDXT1(0, buffer, header.width, header.height, this);
                        callback(buffer, header.width, header.height);
                        break;
                    }

                case 2:
                    {
                        Texture.skipLowLayers(file, header, 16);
                        var buffer = new ArrayBuffer(header.width * header.height * 4);
                        Texture.loadDXT3(0, buffer, header.width, header.height, this);
                        callback(buffer, header.width, header.height);
                        break;
                    }

                case 3:
                    {
                        Texture.skipLowLayers(file, header, 16);
                        var buffer = new ArrayBuffer(header.width * header.height * 4);
                        Texture.loadDXT5(0, buffer, header.width, header.height, this);
                        callback(buffer, header.width, header.height);
                        break;
                    }

                case 0:
                    {
                        Texture.skipLowLayers(file, header, 0);
                        var bytes = file.readBytes(header.width * header.height * 4);
                        callback(bytes.buffer, header.width, header.height);
                        break;
                    }
            }
        };
    })(callback);

    file.open();
}