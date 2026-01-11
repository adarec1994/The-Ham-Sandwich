var M3Model = function (path, geometry) {
    this.EndOfHeader = 0x650;
    this.geometry = geometry;
    this.file = new BinFile(path);
    this.file.onload = (function (self) {
        return function () {
            self.fileOpened();
        };
    })(this);

    this.file.open();
}

M3Model.prototype.fileOpened = function () {
    this.header = this.file.read(M3Model.Header);

    this.file.seek(this.EndOfHeader + this.header.ofsVertices);
    this.vertices = new Float32Array(this.header.nVertices * 8);
    for (var i = 0; i < this.header.nVertices; ++i) {
        var v = this.file.read(M3Model.Vertex);
        this.vertices[i * 8] = v.x;
        this.vertices[i * 8 + 1] = -v.y;
        this.vertices[i * 8 + 2] = v.z;
        this.vertices[i * 8 + 3] = v.nx / 127.0;
        this.vertices[i * 8 + 4] = v.ny / -127.0;
        this.vertices[i * 8 + 5] = v.nz / 127.0;
    }

    var start = this.EndOfHeader + this.header.ofsIndices;
    this.indices = new Uint32Array(this.file.data.buffer, start, this.header.nIndices);
    this.geometry.vertexBuffer.setData(this.vertices);
}

M3Model.Header = [
    'various', 'skip[416]',
    'nTextures', 'u64',
    'ofsTextures', 'u64',
    'various5', 'skip[32]',
    'nMaterials', 'u64',
    'ofsMaterials', 'u64',
    'various2', 'skip[32]',
    'nVertices', 'u64',
    'ofsVertices', 'u64',
    'nIndices', 'u64',
    'ofsIndices', 'u64',
    'nSubMeshes', 'u64',
    'ofsSubMeshes', 'u64',
    'various3', 'skip[56]',
    'nViews', 'u64',
    'ofsViews', 'u64'
];

M3Model.Texture = [
    'unk', 'skip[16]',
    'lenName', 'u64',
    'ofsName', 'u64'
];

M3Model.Vertex = [
    'x', 'f',
    'y', 'f',
    'z', 'f',
    'indices', 'skip[4]',
    'nx', 'i8',
    'ny', 'i8',
    'nz', 'i8',
    'nw', 'i8',
    'tangents', 'skip[4]',
    'unk', 'skip[16]',
    's', 'u16',
    't', 'u16',
    'u', 'u16',
    'v', 'u16'
];

M3Model.SubMesh = [
    'startIndex', 'u32',
    'startVertex', 'u32',
    'nIndices', 'u32',
    'nVertices', 'u32',
    'unk1', 'u32',
    'unk2', 'u16',
    'material', 'u16',
    'unk8', 'u32',
    'color2', 'u32',
    'unk3', 'u32',
    'unk4', 'u32',
    'unk5', 'u32',
    'unk6', 'u32',
    'color3', 'u32',
    'color4', 'u32',
    'unk7', 'u32'
];

M3Model.Skin = [
    'sizeOfStruct', 'u64',
    'nVertexLookup', 'u64',
    'ofsVertexLookup', 'u64',
    'nUnk1', 'u64',
    'ofsUnk1', 'u64',
    'nIndexLookup', 'u64',
    'ofsIndexLookup', 'u64',
    'nUnk2', 'u64',
    'ofsUnk2', 'u64'
];

M3Model.Material = [
    'unk1', 'skip[32]',
    'nTextures', 'u64',
    'ofsTextures', 'u64'
];