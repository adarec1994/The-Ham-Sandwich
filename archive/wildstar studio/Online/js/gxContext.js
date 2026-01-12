var gl = null;

var GxContext = function (canvas) {
    var resizeCanvas = this.onResize.bind(this);

    this.target = canvas;
    this.gl = canvas.getContext('experimental-webgl');
    this.viewportWidth = canvas.width;
    this.viewportHeight = canvas.height;
    $(window).resize(resizeCanvas);
    gl = this.gl;

    var browserPrefixes = [
        "",
        "MOZ_",
        "OP_",
        "WEBKIT_"
    ];


    var supported = gl.getSupportedExtensions();
    var fullName = null;
    for (var i = 0; i < browserPrefixes.length; ++i) {
        var ext = browserPrefixes[i] + 'WEBGL_compressed_texture_s3tc';
        if (supported.indexOf(ext) >= 0) {
            fullName = ext;
        }
    }

    var ext = gl.getExtension(fullName);
    if (ext == null) {
        throw new TypeError("S3TC not supported!");
    }

    GxContext.S3TC = ext;

    Texture.initDefaultTexture();

    this.renderState = null;

    resizeCanvas();
    
    this.gl.clearColor(0.0, 0.0, 0.0, 1.0);

    this.onFrame();
}

GxContext.prototype.onResize = function () {
    var canvas = this.target;

    var width = canvas.clientWidth;
    var height = canvas.clientHeight;
    if (canvas.width != width ||
        canvas.height != height) {
        canvas.width = width;
        canvas.height = height;
    }

    if (textureView) {
        textureView.onResize(canvas.width, canvas.height);
    }
}

GxContext.prototype.onFrame = function () {
    requestAnimFrame(this.onFrame.bind(this));

    this.gl.viewport(0, 0, this.target.width, this.target.height);
    this.gl.clear(this.gl.COLOR_BUFFER_BIT | this.gl.DEPTH_BUFFER_BIT);

    if (this.renderState) {
        this.renderState.onFrame();
    }
}

var VertexBuffer = function () {
    this.buffer = gl.createBuffer();
    this.type = gl.ARRAY_BUFFER;
}

VertexBuffer.prototype.setData = function (data) {
    gl.bindBuffer(this.type, this.buffer);
    gl.bufferData(this.type, data, gl.STATIC_DRAW);
    gl.bindBuffer(this.type, null);
}

VertexBuffer.prototype.bind = function () {
    gl.bindBuffer(this.type, this.buffer);
}

VertexBuffer.prototype.unbind = function () {
    gl.bindBuffer(this.type, null);
}

var IndexBuffer = function () {
    this.buffer = gl.createBuffer();
    this.type = gl.ELEMENT_ARRAY_BUFFER;
    this.indexType = gl.UNSIGNED_SHORT;
}

IndexBuffer.prototype.setData = function (data, bigIndices) {
    gl.bindBuffer(this.type, this.buffer);
    gl.bufferData(this.type, data, gl.STATIC_DRAW);
    gl.bindBuffer(this.type, null);

    if (bigIndices) {
        this.indexType = gl.UNSIGNED_INT;
    } else {
        this.indexType = gl.UNSIGNED_SHORT;
    }
}

IndexBuffer.prototype.bind = function () {
    gl.bindBuffer(this.type, this.buffer);
}

IndexBuffer.prototype.unbind = function () {
    gl.bindBuffer(this.type, null);
}

var VertexSemantic = {
    Position: 'position',
    Normal: 'normal',
    TexCoord: 'texCoord',
    Color: 'color'
};

var DataType = {
    Float: 0,
    Byte: 1
};

var Program = function (vs, fs) {
    var codeVs = $('#' + vs).text();
    var codeFs = $('#' + fs).text();

    this.vShader = gl.createShader(gl.VERTEX_SHADER);
    this.fShader = gl.createShader(gl.FRAGMENT_SHADER);
    this.program = gl.createProgram();

    this.vsCode = codeVs;
    this.fsCode = codeFs;
}

Program.prototype.compile = function (defines) {
    var finalCode = '';
    defines = defines || [];
    for (var i in defines) {
        finalCode += '#define ' + defines[i].name + ' ' + defines[i].value + '\n';
    }

    finalCode += '\n';
    finalCode += this.vsCode;

    gl.shaderSource(this.vShader, finalCode);
    gl.compileShader(this.vShader);

    if (!gl.getShaderParameter(this.vShader, gl.COMPILE_STATUS)) {
        throw new SyntaxError('Unable to compile shader: ' + gl.getShaderInfoLog(this.vShader));
    }

    finalCode = '';
    for (var i in defines) {
        finalCode += '#define ' + defines[i].name + ' ' + defines[i].value + '\n';
    }

    finalCode += '\n';
    finalCode += this.fsCode;

    gl.shaderSource(this.fShader, finalCode);
    gl.compileShader(this.fShader);

    if (!gl.getShaderParameter(this.fShader, gl.COMPILE_STATUS)) {
        throw new SyntaxError('Unable to compile shader: ' + gl.getShaderInfoLog(this.fShader));
    }

    gl.attachShader(this.program, this.vShader);
    gl.attachShader(this.program, this.fShader);
    gl.linkProgram(this.program);

    if (!gl.getProgramParameter(this.program, gl.LINK_STATUS)) {
        throw new SyntaxError('Unable to link program: ' + gl.getProgramInfoLog(this.program));
    }
}

Program.prototype.bind = function () {
    gl.useProgram(this.program);
}

Program.prototype.unbind = function () {
    gl.useProgram(null);
}

Program.prototype.getUniform = function (name) {
    return gl.getUniformLocation(this.program, name);
}

Program.prototype.set = function (nameOrIndex, value) {
    if (nameOrIndex instanceof String) {
        return this.set(this.getUniform(nameOrIndex), value);
    } else {
        this.bind();

        if (value instanceof Number) {
            if ((value % 1) === 0) {
                gl.uniform1i(nameOrIndex, value);
            } else {
                gl.uniform1f(nameOrIndex, value);
            }
        } else if (value instanceof Vector3) {
            gl.uniform3f(nameOrIndex, value.x, value.y, value.z);
        } else if (value instanceof Matrix) {
            gl.uniformMatrix4fv(nameOrIndex, false, value.values);
        }

        this.unbind();
    }
}

var VertexElement = function (semantic, index, numComponents, dataType, normalized) {
    dataType = dataType || DataType.Float;
    normalized = normalized || false;

    this.semantic = semantic;
    this.index = index;
    this.numComponents = numComponents;
    this.dataType = dataType == DataType.Float ? gl.FLOAT : gl.UNSIGNED_BYTE;
    this.normalized = normalized;
}

VertexElement.prototype.bindToProgram = function (program, stride, offset) {
    this.program = program;
    this.offset = offset;
    this.stride = stride;

    var attribName = this.semantic + this.index;
    this.attribIndex = gl.getAttribLocation(program.program, attribName);
}

VertexElement.prototype.bindData = function () {
    gl.enableVertexAttribArray(this.attribIndex);
    gl.vertexAttribPointer(this.attribIndex, this.numComponents, this.dataType, this.normalized, this.stride, this.offset);
}

VertexElement.prototype.unbindData = function () {
    gl.disableVertexAttribArray(this.attribIndex);
}

VertexElement.prototype.getByteSize = function () {
    return this.numComponents * (this.dataType == DataType.Byte ? 1 : 4);
}

var InputGeometry = function () {
    this.program = null;
    this.vertices = 0;
    this.triangles = 0;

    this.vertexBuffer = null;
    this.indexBuffer = null;

    this.stride = 0;
    this.startIndex = 0;
    this.layout = 0;
    this.instances = 0;
    
    this.elements = [];
}

InputGeometry.prototype.addExistingElement = function(elem) {
    this.elements.push(elem);
}

InputGeometry.prototype.addElement = function (semantic, index, numComponents, dataType, normalized) {
    this.addExistingElement(new VertexElement(semantic, index, numComponents, dataType, normalized));
}

InputGeometry.prototype.bindElements = function () {
    if (this.program == null) {
        throw new ReferenceError("Missing program in input geometry");
    }

    if (this.vertexBuffer == null) {
        throw new ReferenceError("No vertex buffer defined");
    }

    this.vertexBuffer.bind();

    for (var i in this.elements) {
        this.elements[i].bindData();
    }
}

InputGeometry.prototype.unbindElements = function () {
    if (this.program == null) {
        throw new ReferenceError("Missing program in input geometry");
    }

    if (this.vertexBuffer == null) {
        throw new ReferenceError("No vertex buffer defined");
    }

    this.vertexBuffer.unbind();

    for (var i in this.elements) {
        this.elements[i].unbindData();
    }
}

InputGeometry.prototype.finalize = function () {
    if (this.program == null) {
        throw new ReferenceError("Missing program in input geometry");
    }

    var offset = 0;

    for (var i in this.elements) {
        var elem = this.elements[i];
        elem.bindToProgram(this.program, this.stride, offset);
        offset += elem.getByteSize();
    }
}

InputGeometry.prototype.getIndexCount = function () {
    return this.triangles * 3;
}

InputGeometry.prototype.getLayout = function () {
    return gl.TRIANGLES;
}

var Pipeline = {
    geometry: null,

    applyGeometry: function (geom) {
        this.geometry = geom;
        geom.bindElements();
    },

    removeGeometry: function (geom) {
        if (this.geometry != geom) {
            return;
        }

        this.geometry.unbindElements();
        this.geometry = null;
    },

    render: function () {
        var g = this.geometry;
        if (g == null || g.vertices == 0 || g.triangles == 0) {
            return;
        }

        var prog = g.program;

        prog.bind();

        g.vertexBuffer.bind();
        g.indexBuffer.bind();

        gl.drawElements(g.getLayout(), g.getIndexCount(), g.indexBuffer.indexType, 0);

        prog.unbind();
        g.vertexBuffer.unbind();
        g.indexBuffer.unbind();
    }
};