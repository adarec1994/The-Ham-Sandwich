var dataViewGlobal = {
    curRot: 0,
    rotInterval: 0
};

var onDataFolderChanged = function(newFolder) {
    $('#dataFolderPath').text("Current Path: " + newFolder);
    $('#dataFilePath').html("<span class='descLabel'>Current file path:</span><br />" + newFolder);
    $('#nonPathSelectedView').css({ 'display': 'none' });
    $('#fileToolbarView').css({ 'display': 'block' });
    $('#dataErrorLabel').css({ display: 'none' });

    dataViewGlobal.rotInterval = setInterval(function() {
        dataViewGlobal.curRot += 6;
        $('#pathLoadRotate').css({ 'transform': 'rotate(' + dataViewGlobal.curRot + 'deg)' });
    }, 16);

    $('#setDataFolderEntry').css({ 'display': 'none' });
    $('#underConstruction').css({ 'display': 'inline-block' });
}

var onUpdateFileLoadProgress = function(cur, total) {
    /*$('#fileLoadProgress').progressbar("value", Math.min(100, (cur / total) * 100.0));
    $('#fileLoadIndicator').text(cur + " / " + total);*/
}

var onFileLoadDone = function(rootNodes) {
    $('#fileToolbarView_load').css({ 'display': 'none' });
    $('#fileToolbarView_tree').css({ 'display': 'block' });
    $('#toolBarExtractButton').removeClass('hidden');

    clearInterval(dataViewGlobal.rotInterval);

    $('#tree').dynatree({
        debugLevel: 0,
        children: rootNodes,
        fx: { height: "toggle", duration: 200 },

        onActivate: function(node) {
            if(node.data.isFolder) {
                return;
            }

            var match = node.data.key.match(/\.([0-9a-z]+)$/i);
            if (match.length <= 1) {
                return;
            }

            var extension = match[1];
            viewController.onFilePress(extension, node.data.key);
        }
    });
}

var onExtractFile = function() {
    var tree = $("#tree").dynatree("getTree");
    var node = tree.getActiveNode();
    if(node == null) {
        $('#errorText').text("No node was selected from the file view. Cannot extract from no node :(");
        $('#errorDialog').dialog('open');
        return;
    }

    var entry = node.data.key;
    viewController.extract(entry);
}

var onAsyncExtractionOperation = function() {
    $('#asyncLoadProgress').progressbar({ value: false });
    $('#asyncDialog').dialog('open');
};

var onAsyncExtractionDone = function() {
    $('#asyncDialog').dialog('close');
};

var parseFileInputs = function (files) {
    var idxFile = null;
    var archFile = null;
    if (files.length != 2) {
        $('#dataErrorLabel').text('Please select ClientData.index AND ClientData.archive in the same dialog.');
        $('#dataErrorLabel').css({ 'visibility': 'visible' });
        return false;
    }

    var m00 = files[0].name.match(/\.index$/i);
    if (m00 != null) {
        idxFile = files[0];
    } else {
        m01 = files[1].name.match(/\.index$/i);
        if (m01 != null) {
            idxFile = files[1];
        }
    }

    var m10 = files[0].name.match(/\.archive$/i);
    if (m10 != null) {
        archFile = files[0];
    } else {
        m11 = files[1].name.match(/\.archive$/i);
        if (m11 != null) {
            archFile = files[1];
        }
    }

    if (idxFile == null || archFile == null) {
        $('#dataErrorLabel').text('Please select ClientData.index AND ClientData.archive in the same dialog.');
        $('#dataErrorLabel').css({ 'visibility': 'visible' });
        return false;
    }

    return ArchiveManager.initialize(idxFile, archFile);
};

var FileEntry = function (parent, name, dataBlob) {
    this.data = dataBlob;
    this.entryName = name;
    this.parent = parent;
    this.entryNameLower = name.toLowerCase();
    this.fullPath = ((parent && parent.isRoot == false) ? (parent.fullPath + '\\') : '') + name;
    this.isFolder = false;
    var view = new DataView(dataBlob);
    this.flags = view.getUint32(0, true);
    this.uncompressed = view.getUint32(12, true);
    this.uncompressed += view.getUint32(16, true) * Math.pow(2, 32);
    this.compressed = view.getUint32(20, true);
    this.compressed += view.getUint32(24, true) * Math.pow(2, 32);
    this.hash = [
        view.getUint32(28, true),
        view.getUint32(32, true),
        view.getUint32(36, true),
        view.getUint32(40, true),
        view.getUint32(44, true)
    ];
}

FileEntry.prototype.getFullPath = function () {
    return this.fullPath;
}

var DirectoryEntry = function (parent, name, nextBlock) {
    this.nextBlock = nextBlock;
    this.isRoot = parent == null;
    this.parent = parent;
    this.entryName = name;
    this.entryNameLower = name.toLowerCase();
    this.children = [];
    this.subDirs = [];
    this.fullPath = ((parent && parent.isRoot == false) ? (parent.fullPath + '\\') : '') + name;
    this.isFolder = true;
};

DirectoryEntry.prototype.readDataName = function (index, array) {
    var str = '';
    for (var i = index; i < array.byteLength; ++i) {
        if (array.getUint8(i) == 0) {
            return str;
        }

        str += String.fromCharCode(array.getUint8(i));
    }

    return str;
}

DirectoryEntry.prototype.parseChildren = function (onComplete) {
    var nextBlock = ArchiveManager.directoryHeaders[this.nextBlock];
    var idx = ArchiveManager.indexFile;
    idx.seek(nextBlock.offset);
    idx.readBytes(nextBlock.size, (function (self, idx, nextBlock, _complete) {
        return function (view, buffer) {
            var numDirs = view.getUint32(0, true);
            var numFiles = view.getUint32(4, true);

            var dataSize = numDirs * 8 + numFiles * 56 + 8;
            for (var i = 0; i < numDirs; ++i) {
                var nameOffset = view.getUint32(8 + i * 8, true);
                var nextDir = view.getUint32(8 + i * 8 + 4, true);
                var name = self.readDataName(nameOffset + dataSize, view);
                var dirEnt = new DirectoryEntry(self, name, nextDir);
                self.subDirs.push(dirEnt);
                self.children.push(dirEnt);
            }

            for (var i = 0; i < numFiles; ++i) {
                var nameOffset = view.getUint32(numDirs * 8 + 8 + i * 56, true);
                var name = self.readDataName(nameOffset + dataSize, view);
                var fileData = buffer.slice(numDirs * 8 + 8 + i * 56 + 4, numDirs * 8 + 8 + (i + 1) * 56);
                var fileEntry = new FileEntry(self, name, fileData);
                self.children.push(fileEntry);
            }

            var endParse = function () {
                var _this = self;
                var complete = _complete;
                var subParser = function (index) {
                    if (index == _this.subDirs.length) {
                        _this.subDirs = [];
                        complete();
                        return;
                    }

                    _this.subDirs[index].parseChildren((function (_this, complete, index) {
                        return function () {
                            subParser(index + 1);
                        };
                    })(_this, complete, index));
                };

                subParser(0);
            }

            endParse();
        };
    })(this, idx, nextBlock, onComplete));
}

DirectoryEntry.prototype.getFile = function (path) {
    var parts = path.split('\\');
    var remPath = parts.slice(1).join('\\');
    if (parts.length == 1) {
        for (var i in this.children) {
            var child = this.children[i];
            if (child.entryNameLower == path) {
                return child;
            }
        }

        return null;
    }

    var dir = parts[0];
    for (var i = 0; i < this.children.length; ++i) {
        var child = this.children[i];
        if (child instanceof DirectoryEntry && child.entryNameLower == dir) {
            return child.getFile(remPath);
        }
    }

    return null;
}

DirectoryEntry.prototype.getFullPath = function () {
    return this.fullPath;
}

DirectoryEntry.prototype.getEntries = function (recursive) {
    var ret = this.children.slice(0);
    if (recursive) {
        for (var i = 0; i < this.children.length; ++i) {
            if (this.children[i] instanceof DirectoryEntry) {
                ret.concat(this.children[i].getEntries(true));
            }
        }
    }

    return ret;
}

var ArchiveManager = {
    archiveFile: null,
    indexFile: null,
    directoryHeaders: [],
    fileRoot: null,
    validIconExts: [
        'area', 'bnk', 'form', 'i3', 'jpg', 'lua', 'm3', 'psd', 'sho', 'tbl', 'tex', 'txd', 'wem', 'xml'
    ],
    pkDirectoryHeaders: [],
    fileHeaders: [],

    onFileError: function(message) {
        message = message || 'Unable to parse archive files.';

        $('#dataErrorLabel').text(message);
        $('#dataErrorLabel').css({ 'visibility': 'visible' });
        $(document).focus();
    },

    initialize: function (index, archive) {
        this.archiveFile = new BlobFile(archive);
        this.indexFile = new BlobFile(index);

        this.indexFile.readUInt32(function (sig) {
            if (sig != 0x5041434B) {
                ArchiveManager.onFileError('Index file is not a valid wildstar index file.');
                return;
            }

            ArchiveManager.loadIndexFile();
        }, ArchiveManager.onFileError);

        return true;
    },

    loadIndexFile: function () {
        onDataFolderChanged('ClientData.index');
        var self = this;
        this.indexFile.skip(532);
        this.indexFile.readBytes(12, function (view) {
            var dirTableStart = view.getUint32(0, true);
            dirTableStart += view.getUint32(4, true) * Math.pow(2, 32);
            var dirCount = view.getUint32(8, true);
            self.indexFile.seek(dirTableStart);
            self.indexFile.readBytes(dirCount * 16, function (view) {
                var numEntries = view.byteLength / 16;
                for (var i = 0; i < numEntries; ++i) {
                    var offset = view.getUint32(i * 16, true);
                    offset += view.getUint32(i * 16 + 4, true) * Math.pow(2, 32);
                    var size = view.getUint32(i * 16 + 8, true);
                    size += view.getUint32(i * 16 + 12, true) * Math.pow(2, 32);
                    ArchiveManager.directoryHeaders.push({ offset: offset, size: size });
                }

                self.loadIndexTree();
            }, function () { ArchiveManager.onFileError('Unable to parse directory table'); });
        }, function () { ArchiveManager.onFileError('Unable to parse directory table'); } );
    },

    loadIndexTree: function () {
        if (this.directoryHeaders.length == 0) {
            return;
        }

        var self = this;

        var parser = function (index) {
            if (index == self.directoryHeaders.length) {
                return;
            }

            var entry = self.directoryHeaders[index];
            self.indexFile.seek(entry.offset);
            if (entry.size < 16) {
                parser(index + 1);
                return;
            }

            self.indexFile.readBytes(16, (function(index) { return function (value) {
                var magic = value.getUint32(0, true);
                var version = value.getUint32(4, true);
                var unk1 = value.getUint32(8, true);
                var rootBlock = value.getUint32(12, true);

                if (magic == 0x41494458) {
                    ArchiveManager.loadIndexRoot(rootBlock);
                    return;
                }

                parser(index + 1);
            };})(index), (function (index) { return function () { parser(index + 1); }; })(index));
        };

        parser(0);
    },

    loadIndexRoot: function (rootBlock) {
        ArchiveManager.fileRoot = new DirectoryEntry(null, '', rootBlock);
        ArchiveManager.fileRoot.parseChildren(function () {
            ArchiveManager.loadDataFile();
        });
    },

    loadDataTree: function () {
        if (this.pkDirectoryHeaders.length == 0) {
            return;
        }

        var self = this;
        var parser = function (index) {
            if (index == self.pkDirectoryHeaders.length) {
                return;
            }

            var entry = self.pkDirectoryHeaders[index];
            self.archiveFile.seek(entry.offset);
            if (entry.size < 16) {
                parser(index + 1);
                return;
            }

            self.archiveFile.readBytes(16, (function (index) {
                return function (value) {
                    var magic = value.getUint32(0, true);
                    var version = value.getUint32(4, true);
                    var entryCount = value.getUint32(8, true);
                    var rootBlock = value.getUint32(12, true);

                    if (magic == 0x41415243) {
                        ArchiveManager.loadDataRoot(rootBlock, entryCount);
                        return;
                    }

                    parser(index + 1);
                };
            })(index), (function (index) { return function () { parser(index + 1); }; })(index));
        };

        parser(0);
    },

    loadDataRoot: function (rootBlock, numEntries) {
        var block = this.pkDirectoryHeaders[rootBlock];
        this.archiveFile.seek(block.offset);
        var byteCount = numEntries * 32;
        this.archiveFile.readBytes(byteCount, (function (self, numEntries) {
            return function (view) {
                for (var i = 0; i < numEntries; ++i) {
                    var block = view.getUint32(i * 32, true);
                    var hash1 = view.getUint32(i * 32 + 4, true);
                    var hash2 = view.getUint32(i * 32 + 8, true);
                    var hash3 = view.getUint32(i * 32 + 12, true);
                    var hash4 = view.getUint32(i * 32 + 16, true);
                    var hash5 = view.getUint32(i * 32 + 20, true);
                    var uncompressed = view.getUint32(i * 32 + 24, true);
                    uncompressed += view.getUint32(i * 32 + 28, true) * Math.pow(2, 32);
                    self.fileHeaders.push({
                        block: block,
                        uncompressed: uncompressed,
                        hash: [hash1, hash2, hash3, hash4, hash5]
                    });
                }

                onFileLoadDone(ArchiveManager.buildChildNodes(ArchiveManager.fileRoot));
            };
        })(this, numEntries));
    },

    loadDataFile: function () {
        this.archiveFile.readBytes(556, (function (self) {
            return function (view) {
                var sig = view.getUint32(0, true);
                if (sig != 0x5041434B) {
                    self.onFileError('Invalid archive file (signature mismatch)');
                    return;
                }

                var pkDirCount = view.getUint32(544, true);
                var pkOffset = view.getUint32(536, true);
                pkOffset += view.getUint32(540, true) * Math.pow(2, 32);
                self.archiveFile.seek(pkOffset);
                
                var count = 16 * pkDirCount;
                self.archiveFile.readBytes(count, (function (self, pkDirCount) {
                    return function (view) {
                        for (var i = 0; i < pkDirCount; ++i) {
                            var offset = view.getUint32(i * 16, true);
                            offset += (view.getUint32(i * 16 + 4, true)) * Math.pow(2, 32);
                            var size = view.getUint32(i * 16 + 8, true);
                            size += view.getUint32(i * 16 + 12, true) * Math.pow(2, 32);
                            self.pkDirectoryHeaders.push({ offset: offset, size: size });
                        }

                        self.loadDataTree();
                    };
                })(self, pkDirCount));
            };
        })(this));
    },

    getFileData: function(file, success, error) {
        if (file instanceof FileEntry) {
            for (var i = 0; i < this.fileHeaders.length; ++i) {
                var header = this.fileHeaders[i];
                var found = true;
                for (var j = 0; j < 5; ++j) {
                    if (header.hash[j] != file.hash[j]) {
                        found = false;
                        break;
                    }
                }

                if (found) {
                    var block = this.pkDirectoryHeaders[header.block];
                    this.archiveFile.seek(block.offset);
                    this.archiveFile.readBytes(block.size, (function (self, file, header, success) {
                        return function (view, buffer) {
                            if (file.flags != 3) {
                                if (success) {
                                    success(view);
                                }
                            } else {
                                var data = zlib.inflate(buffer, file.uncompressed);
                                success(new DataView(data.buffer));
                            }
                        };
                    })(this, file, header, success), function () { if (error) error(); });
                    return;
                }
            }

            if (error) {
                error();
            }
        } else {
            if (error) {
                error();
            }
        }
    },

    getByPath: function (path) {
        return this.fileRoot.getFile(path.toLowerCase());
    },

    buildChildNodes: function (node) {
        var entries = node.getEntries();
        var nodes = [];
        for (var i in entries) {
            var subNode = entries[i];
            var entry = {
                title: subNode.entryName,
                key: subNode.getFullPath(),
                isFolder: subNode instanceof DirectoryEntry,
            };

            if (entry.isFolder == false) {
                var match = entry.key.match(/\.([0-9a-z]+)$/i);
                if (match != null && match.length > 0) {
                    var ext = match[1];
                    if (this.validIconExts.indexOf(ext.toLowerCase())) {
                        entry.icon = ext + '.png'
                    }
                }
            } else {
                entry.children = this.buildChildNodes(subNode);
            }

            nodes.push(entry);
        }

        return nodes;
    }
}

var BinFile = function (path) {
    this.filePath = path;
    this.file = ArchiveManager.getByPath(path);
    if (this.file == null) {
        throw new URIError('File ' + path + ' not found!');
    }
}

BinFile.prototype.open = function () {
    ArchiveManager.getFileData(this.file, (function (self) {
        return function (data) {
            self.data = data;
            self.position = 0;
            self.fileSize = self.data.byteLength;
            if (self.onload) {
                self.onload.apply(self);
            }
        };
    })(this), (function (self) {
        return function () {
            if (self.onerror) {
                self.onerror.apply(self);
            }
        };
    })(this));
}

BinFile.prototype.checkOpen = function () {
    if (this.data == null) {
        throw new ReferenceError('File is not opened yet, call BinFile.open first');
    }
}

BinFile.prototype.seek = function (offset) {
    this.position = offset;
}

BinFile.prototype.seekMod = function (offset) {
    this.position += offset;
}

BinFile.prototype.readBytes = function (numBytes) {
    this.checkOpen();

    var buf = new Uint8Array(this.data.buffer, this.position, numBytes);
    this.position += numBytes;
    return buf;
}

BinFile.prototype.read = function (descriptor) {
    if (descriptor.length == null || descriptor.length % 2) {
        return;
    }

    var obj = { };

    for (var i = 0; i < descriptor.length; i += 2) {
        var name = descriptor[i];
        var type = descriptor[i + 1];
        type = type.toLowerCase();
        var parsed = true;

        switch (type) {
            case 'u64':
                {
                    var lower = this.readUint32();
                    var upper = this.readUint32();
                    obj[name] = lower + upper * Math.pow(2, 32);
                }
                break;

            case 'u32':
                obj[name] = this.readUint32();
                break;

            case 'i32':
                obj[name] = this.readInt32();
                break;

            case 'u16':
                obj[name] = this.readUint16();
                break;

            case 'i16':
                obj[name] = this.readInt16();
                break;

            case 'u8':
                obj[name] = this.readUint8();
                break;
                
            case 'i8':
                obj[name] = this.readInt8();
                break;

            case 's':
                obj[name] = this.readString();
                break;

            case 'us':
                obj[name] = this.readStringWide();
                break;

            case 'f':
                obj[name] = this.readFloat();
                break;

            case 'd':
                obj[name] = this.readDouble();
                break;

            default:
                parsed = false;
                break;
        }

        if (!parsed) {
            var idxArray = type.indexOf('[');
            if (idxArray > 0) {
                var endArray = type.indexOf(']', idxArray);
                var num = parseInt(type.substr(idxArray + 1, endArray - 1));
                if (num == NaN) {
                    throw new SyntaxError("invalid type descriptor: " + type);
                }

                var arrayType = type.substr(0, idxArray);
                if (arrayType == 'skip') {
                    this.position += num;
                } else {
                    obj[name] = [];
                    for (var i = 0; i < num; ++i) {
                        obj[name].push(this.read(['value', arrayType]).value);
                    }
                }
            } else {
                throw new SyntaxError("invalid type descriptor: " + type);
            }
        }
    }

    return obj;
}

BinFile.prototype.readStringWide = function (len) {
    var ret = '';
    if (len) {
        for (var i = 0; i < len; ++i) {
            var cur = this.readUint16();
            if (cur == 0) {
                break;
            }

            ret += String.fromCharCode(cur);
        }
    } else {
        var valid = true;
        while (valid) {
            var cur = this.readUint16();
            if (cur == 0) {
                break;
            }

            ret += String.fromCharCode(cur);
        }
    }

    return ret;
}

BinFile.prototype.readString = function (len) {
    var ret = '';
    if (len) {
        for (var i = 0; i < len; ++i) {
            var cur = this.readUint8();
            if (cur == 0) {
                break;
            }

            ret += String.fromCharCode(cur);
        }
    } else {
        var valid = true;
        while (valid) {
            var cur = this.readUint8();
            if (cur == 0) {
                break;
            }

            ret += String.fromCharCode(cur);
        }
    }

    return ret;
}

BinFile.prototype.readUint64 = function () {
    var lower = this.readUint32();
    var upper = this.readUint32();
    return lower + Math.pow(2, 32) * upper;
}

BinFile.prototype.readInt64 = function () {
    var lower = this.readUint32();
    var upper = this.readUint32();
    if (upper & 0x80000000) {
        lower = ~lower;
        upper = ~upper;
        var carry = lower & 0x80000000;
        lower += 1;
        if (carry) {
            upper += 1;
        }

        return -1 * (lower + upper * Math.pow(2, 32));
    }

    return lower + Math.pow(2, 32) * upper;
}

BinFile.prototype.readUint32 = function () {
    this.checkOpen();
    var ret = this.data.getUint32(this.position, true);
    this.position += 4;
    return ret;
}

BinFile.prototype.readInt32 = function () {
    this.checkOpen();
    var ret = this.data.getInt32(this.position, true);
    this.position += 4;
    return ret;
}

BinFile.prototype.readUint16 = function () {
    this.checkOpen();
    var ret = this.data.getUint16(this.position, true);
    this.position += 2;
    return ret;
}

BinFile.prototype.readInt16 = function () {
    this.checkOpen();
    var ret = this.data.getInt16(this.position, true);
    this.position += 2;
    return ret;
}

BinFile.prototype.readUint8 = function () {
    this.checkOpen();
    var ret = this.data.getUint8(this.position);
    this.position += 1;
    return ret;
}

BinFile.prototype.readInt8 = function () {
    this.checkOpen();
    var ret = this.data.getInt8(this.position);
    this.position += 1;
    return ret;
}

BinFile.prototype.readFloat = function () {
    this.checkOpen();
    var ret = this.data.getFloat32(this.position, true);
    this.position += 4;
    return ret;
}

BinFile.prototype.readDouble = function () {
    this.checkOpen();
    var ret = this.data.getFloat64(this.position, true);
    this.position += 8;
    return ret;
}

var BlobFile = function (file) {
    this.blobFile = file;
    this.reader = new FileReader();
    this.curPosition = 0;
    this.onSuccess = null;
    this.onError = null;
    var self = this;
    this.reader.onerror = function () {
        if (self.onError) {
            self.onError();
        }
    };

    this.reader.onload = function (e) {
        if (self.onSuccess) {
            self.onSuccess(e.target.result);
        }
    };
}

BlobFile.prototype.readUInt32 = function (onSuccess, onError) {
    if (this.curPosition + 4 > this.blobFile.size) {
        if (onError) {
            onError("End of stream reached");
        }
        return;
    }

    var slice = this.blobFile.slice(this.curPosition, this.curPosition + 4);
    this.curPosition += 4;
    this.onError = onError;
    this.onSuccess = (function (onSuccess) {
        return function (buffer) {
            if (onSuccess) {
                var arr = new Uint32Array(buffer);
                onSuccess(arr[0]);
            }
        };
    })(onSuccess);

    this.reader.readAsArrayBuffer(slice);
}

BlobFile.prototype.readUInt64 = function (onSuccess, onError) {
    if (this.curPosition + 8 > this.blobFile.size) {
        if (onError) {
            onError("End of stream reached");
        }
        return;
    }

    var slice = this.blobFile.slice(this.curPosition, this.curPosition + 8);
    this.curPosition += 8;
    this.onError = onError;
    this.onSuccess = (function (onSuccess) {
        return function (buffer) {
            if (onSuccess) {
                var arr = new Uint32Array(buffer);
                var v0 = arr[0];
                var v1 = arr[1];
                onSuccess(v0 | (v1 << 32));
            }
        };
    })(onSuccess);

    this.reader.readAsArrayBuffer(slice);
}

BlobFile.prototype.seek = function (offset) {
    this.curPosition = offset;
}

BlobFile.prototype.skip = function (skip) {
    this.curPosition += skip;
}

BlobFile.prototype.readBytes = function (numBytes, onSuccess, onError) {
    if (this.curPosition + numBytes > this.blobFile.size) {
        if (onError) {
            onError("End of stream reached");
        }
        return;
    }

    var slice = this.blobFile.slice(this.curPosition, this.curPosition + numBytes);
    this.curPosition += numBytes;
    this.onError = onError;
    this.onSuccess = (function (onSuccess) {
        return function (buffer) {
            if (onSuccess) {
                onSuccess(new DataView(buffer), buffer);
            }
        };
    })(onSuccess);

    this.reader.readAsArrayBuffer(slice);
}

$('document').ready(function() {
    $('#setDataFolderEntry').click(function() {
        API.filesystem.setPath();
    });

    $('#selectDataFolderToolEntry').click(function() {
        $('#setDataFolderEntry input').click();
    });

    $('#toolBarExtractButton').click(function() {
        onExtractFile();
    });

    $('#toolBarExtractButton').tooltip({
        position: {
            my: "center bottom-15",
            at: "center top",
            using: function(position, feedback) {
                $(this).css(position);
                $("<div>")
                    .addClass( "arrow" )
                    .addClass( feedback.vertical )
                    .addClass( feedback.horizontal )
                    .appendTo( this );
            }
        }
    });

    $('#setDataFolderEntry input').change(function () {
        if (parseFileInputs(this.files) == false) {
            return;
        }
    });


    $('#fileLoadProgress').progressbar({
        value: false
    });
});

var zlib = {
    internal_inflate: Module.cwrap('cromon_inflate', 'number', ['number', 'number', 'number', 'number']),

    inflate: function (data, uncompressedSize) {
        var outputPtr = Module._malloc(uncompressedSize);
        var inputPtr = Module._malloc(data.byteLength);
        var inputHeap = new Uint8Array(Module.HEAPU8.buffer, inputPtr, data.byteLength);
        var outputHeap = new Uint8Array(Module.HEAPU8.buffer, outputPtr, uncompressedSize);
        inputHeap.set(new Uint8Array(data));

        var ret = zlib.internal_inflate(inputHeap.byteOffset, data.byteLength, outputHeap.byteOffset, uncompressedSize);
        if (ret < 0) {
            throw new Error('ZLib error: ' + ret);
        }

        var data = new Uint8Array(uncompressedSize);
        data.set(outputHeap);
        Module._free(outputHeap.byteOffset);
        Module._free(inputHeap.byteOffset);

        return data;
    }
}