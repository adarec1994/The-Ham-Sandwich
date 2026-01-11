var textureView = {
    textureObject: null,
    geometry: null,

    show: function (texName) {
        if (textureView.geometry == null) {
            textureView.initGeometry();
        }

        $('#glDrawArea').css({ display: 'block' });

        textureView.matView = Matrix.ortho(0, 0, gl.viewportWidth, gl.viewportHeight);

        this.setObject(texName);
        $('#textureGlView').css({ 'display': 'block' });
    },

    hide: function() {
        $('#textureGlView').css({ 'display': 'none' });
        if (API.renderer.renderState == textureView) {
            API.renderer.renderState = null;
        }
    },

    setObject: function(texObj) {
        this.textureObject = texObj;
        $('#textureNameDesc').text(texObj);

        var tex = new Texture(texObj);
        this.texture = tex;
        tex.onload = function () {
            $('#textureSizeDesc').text(this.header.width.toString() + ' x ' + this.header.height);
            var width = Math.min(this.header.width, API.renderer.target.width - 400);
            var height = Math.min(this.header.height, API.renderer.target.height - 50);

            var aspect = this.header.width / this.header.height;
            var scalew = width / this.header.width;
            var scaleh = height / this.header.height;
            if (scalew < scaleh) {
                height = width / aspect;
            } else {
                width = aspect * height;
            }

            var vertices = new Float32Array([
                400, 25, 0, 0, 0,
                400 + width, 25, 0, 1, 0,
                400 + width, 25 + height, 0, 1, 1,
                400, 25 + height, 0, 0, 1
            ]);

            textureView.geometry.vertexBuffer.setData(vertices);

            API.renderer.renderState = textureView;
        };

        tex.load();
    },

    onFrame: function () {
        this.program.set(this.uniformTexture, 0);
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, this.texture.id);

        gl.disable(gl.DEPTH_TEST);
        gl.disable(gl.BLEND);

        Pipeline.applyGeometry(this.geometry);
        Pipeline.render();
        Pipeline.removeGeometry(this.geometry);

        gl.bindTexture(gl.TEXTURE_2D, null);
    },

    onPropertiesLoaded: function(props) {
        $('#textureSizeDesc').text(props.width + "x" + props.height);
    },

    onResize: function (width, height) {
        textureView.matView = Matrix.ortho(0, 0, width, height, 0, 1);
        if (textureView.program) {
            textureView.program.set(textureView.uniformMatrix, textureView.matView);
        }

        if (this.texture) {
            var width = Math.min(this.texture.header.width, API.renderer.target.width - 400);
            var height = Math.min(this.texture.header.height, API.renderer.target.height - 50);

            var aspect = this.texture.header.width / this.texture.header.height;
            var scalew = width / this.texture.header.width;
            var scaleh = height / this.texture.header.height;
            if (scalew < scaleh) {
                height = width / aspect;
            } else {
                width = aspect * height;
            }

            var vertices = new Float32Array([
                400, 25, 0, 0, 0,
                400 + width, 25, 0, 1, 0,
                400 + width, 25 + height, 0, 1, 1,
                400, 25 + height, 0, 0, 1
            ]);

            textureView.geometry.vertexBuffer.setData(vertices);
        }
    },

    initGeometry: function () {
        this.geometry = new InputGeometry();
        this.geometry.addElement(VertexSemantic.Position, 0, 3);
        this.geometry.addElement(VertexSemantic.TexCoord, 0, 2);
        this.geometry.stride = 20;
        this.geometry.vertices = 6;
        this.geometry.triangles = 2;

        this.geometry.indexBuffer = new IndexBuffer();
        this.geometry.vertexBuffer = new VertexBuffer();

        this.geometry.indexBuffer.setData(new Uint16Array([0, 1, 2, 0, 2, 3]), false);
        this.program = new Program('quadVertex', 'quadFragment');
        this.program.compile();
        this.uniformTexture = this.program.getUniform('texture0');
        this.uniformMatrix = this.program.getUniform('matProj');
        if (this.matView) {
            this.program.set(this.uniformMatrix, this.matView);
        }

        this.geometry.program = this.program;

        this.geometry.finalize();
    },

    extract: function (entry) {
        if (!API.filesystem.textureAsBmp()) {
            var file = new BinFile(entry);
            file.onload = function () {
                var path = this.filePath;
                path = path.replace(/^.*[\\\/]/, '');
                var bytes = this.readBytes(this.fileSize);
                var blob = new Blob([bytes], { type: 'application/octet-stream' });
                var url = window.URL.createObjectURL(blob);
                var a = document.createElement('a');
                a.setAttribute('href', url);
                a.setAttribute('download', path);
                var event = document.createEvent('MouseEvents');
                event.initMouseEvent('click', true, true, window, 1, 0, 0, 0, 0, false, false, false, false, 0, null);
                a.dispatchEvent(event);
            };

            file.open();
        } else {
            Texture.getBitmapMemory(entry, (function (entry) {
                return function (buffer, width, height) {
                    entry = entry.replace(/^.*[\\\/]/, '');
                    entry = entry.replace(/\.[0-9a-z]+$/i, '');
                    entry += '.bmp';
                    var header = new DataView(new ArrayBuffer(54));
                    header.setUint16(0, 0x4D42, true);
                    header.setUint32(2, buffer.byteLength + 54, true);
                    header.setUint32(6, 0, true);
                    header.setUint32(10, 54, true);
                    header.setUint32(14, 40, true);
                    header.setInt32(18, width, true);
                    header.setInt32(22, -height, true);
                    header.setUint16(26, 1, true);
                    header.setUint16(28, 32, true);
                    header.setUint32(30, 0, true);
                    header.setUint32(34, buffer.byteLength, true);
                    header.setUint32(38, 0, true);
                    header.setUint32(42, 0, true);
                    header.setUint32(46, 0, true);
                    header.setUint32(50, 0, true);

                    var blob = new Blob([header.buffer, buffer], { type: 'image/bmp' });
                    var url = window.URL.createObjectURL(blob);
                    var a = document.createElement('a');
                    a.setAttribute('href', url);
                    a.setAttribute('download', entry);
                    var event = document.createEvent('MouseEvents');
                    event.initMouseEvent('click', true, true, window, 1, 0, 0, 0, 0, false, false, false, false, 0, null);
                    a.dispatchEvent(event);
                };
            })(entry));
        }
    }
}

viewController.extensionMap.tex = textureView;

$('document').ready(function() {
    $('#exportTextureButton').click(function() {
        if (textureView.textureObject) {
            textureView.extract(textureView.textureObject);
        }
    });
});