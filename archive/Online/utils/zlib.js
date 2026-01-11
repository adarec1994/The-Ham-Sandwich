// The Module object: Our interface to the outside world. We import
// and export values on it, and do the work to get that through
// closure compiler if necessary. There are various ways Module can be used:
// 1. Not defined. We create it here
// 2. A function parameter, function(Module) { ..generated code.. }
// 3. pre-run appended it, var Module = {}; ..generated code..
// 4. External script tag defines var Module.
// We need to do an eval in order to handle the closure compiler
// case, where this code here is minified but Module was defined
// elsewhere (e.g. case 4 above). We also need to check if Module
// already exists (e.g. case 3 above).
// Note that if you want to run closure, and also to use Module
// after the generated code, you will need to define   var Module = {};
// before the code. Then that object will be used in the code, and you
// can continue to use Module afterwards as well.
var Module;
if (!Module) Module = (typeof Module !== 'undefined' ? Module : null) || {};

// Sometimes an existing Module object exists with properties
// meant to overwrite the default module functionality. Here
// we collect those properties and reapply _after_ we configure
// the current environment's defaults to avoid having to be so
// defensive during initialization.
var moduleOverrides = {};
for (var key in Module) {
  if (Module.hasOwnProperty(key)) {
    moduleOverrides[key] = Module[key];
  }
}

// The environment setup code below is customized to use Module.
// *** Environment setup code ***
var ENVIRONMENT_IS_NODE = typeof process === 'object' && typeof require === 'function';
var ENVIRONMENT_IS_WEB = typeof window === 'object';
var ENVIRONMENT_IS_WORKER = typeof importScripts === 'function';
var ENVIRONMENT_IS_SHELL = !ENVIRONMENT_IS_WEB && !ENVIRONMENT_IS_NODE && !ENVIRONMENT_IS_WORKER;

if (ENVIRONMENT_IS_NODE) {
  // Expose functionality in the same simple way that the shells work
  // Note that we pollute the global namespace here, otherwise we break in node
  if (!Module['print']) Module['print'] = function print(x) {
    process['stdout'].write(x + '\n');
  };
  if (!Module['printErr']) Module['printErr'] = function printErr(x) {
    process['stderr'].write(x + '\n');
  };

  var nodeFS = require('fs');
  var nodePath = require('path');

  Module['read'] = function read(filename, binary) {
    filename = nodePath['normalize'](filename);
    var ret = nodeFS['readFileSync'](filename);
    // The path is absolute if the normalized version is the same as the resolved.
    if (!ret && filename != nodePath['resolve'](filename)) {
      filename = path.join(__dirname, '..', 'src', filename);
      ret = nodeFS['readFileSync'](filename);
    }
    if (ret && !binary) ret = ret.toString();
    return ret;
  };

  Module['readBinary'] = function readBinary(filename) { return Module['read'](filename, true) };

  Module['load'] = function load(f) {
    globalEval(read(f));
  };

  Module['arguments'] = process['argv'].slice(2);

  module['exports'] = Module;
}
else if (ENVIRONMENT_IS_SHELL) {
  if (!Module['print']) Module['print'] = print;
  if (typeof printErr != 'undefined') Module['printErr'] = printErr; // not present in v8 or older sm

  if (typeof read != 'undefined') {
    Module['read'] = read;
  } else {
    Module['read'] = function read() { throw 'no read() available (jsc?)' };
  }

  Module['readBinary'] = function readBinary(f) {
    return read(f, 'binary');
  };

  if (typeof scriptArgs != 'undefined') {
    Module['arguments'] = scriptArgs;
  } else if (typeof arguments != 'undefined') {
    Module['arguments'] = arguments;
  }

  this['Module'] = Module;

  eval("if (typeof gc === 'function' && gc.toString().indexOf('[native code]') > 0) var gc = undefined"); // wipe out the SpiderMonkey shell 'gc' function, which can confuse closure (uses it as a minified name, and it is then initted to a non-falsey value unexpectedly)
}
else if (ENVIRONMENT_IS_WEB || ENVIRONMENT_IS_WORKER) {
  Module['read'] = function read(url) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, false);
    xhr.send(null);
    return xhr.responseText;
  };

  if (typeof arguments != 'undefined') {
    Module['arguments'] = arguments;
  }

  if (typeof console !== 'undefined') {
    if (!Module['print']) Module['print'] = function print(x) {
      console.log(x);
    };
    if (!Module['printErr']) Module['printErr'] = function printErr(x) {
      console.log(x);
    };
  } else {
    // Probably a worker, and without console.log. We can do very little here...
    var TRY_USE_DUMP = false;
    if (!Module['print']) Module['print'] = (TRY_USE_DUMP && (typeof(dump) !== "undefined") ? (function(x) {
      dump(x);
    }) : (function(x) {
      // self.postMessage(x); // enable this if you want stdout to be sent as messages
    }));
  }

  if (ENVIRONMENT_IS_WEB) {
    window['Module'] = Module;
  } else {
    Module['load'] = importScripts;
  }
}
else {
  // Unreachable because SHELL is dependant on the others
  throw 'Unknown runtime environment. Where are we?';
}

function globalEval(x) {
  eval.call(null, x);
}
if (!Module['load'] == 'undefined' && Module['read']) {
  Module['load'] = function load(f) {
    globalEval(Module['read'](f));
  };
}
if (!Module['print']) {
  Module['print'] = function(){};
}
if (!Module['printErr']) {
  Module['printErr'] = Module['print'];
}
if (!Module['arguments']) {
  Module['arguments'] = [];
}
// *** Environment setup code ***

// Closure helpers
Module.print = Module['print'];
Module.printErr = Module['printErr'];

// Callbacks
Module['preRun'] = [];
Module['postRun'] = [];

// Merge back in the overrides
for (var key in moduleOverrides) {
  if (moduleOverrides.hasOwnProperty(key)) {
    Module[key] = moduleOverrides[key];
  }
}



// === Auto-generated preamble library stuff ===

//========================================
// Runtime code shared with compiler
//========================================

var Runtime = {
  stackSave: function () {
    return STACKTOP;
  },
  stackRestore: function (stackTop) {
    STACKTOP = stackTop;
  },
  forceAlign: function (target, quantum) {
    quantum = quantum || 4;
    if (quantum == 1) return target;
    if (isNumber(target) && isNumber(quantum)) {
      return Math.ceil(target/quantum)*quantum;
    } else if (isNumber(quantum) && isPowerOfTwo(quantum)) {
      return '(((' +target + ')+' + (quantum-1) + ')&' + -quantum + ')';
    }
    return 'Math.ceil((' + target + ')/' + quantum + ')*' + quantum;
  },
  isNumberType: function (type) {
    return type in Runtime.INT_TYPES || type in Runtime.FLOAT_TYPES;
  },
  isPointerType: function isPointerType(type) {
  return type[type.length-1] == '*';
},
  isStructType: function isStructType(type) {
  if (isPointerType(type)) return false;
  if (isArrayType(type)) return true;
  if (/<?\{ ?[^}]* ?\}>?/.test(type)) return true; // { i32, i8 } etc. - anonymous struct types
  // See comment in isStructPointerType()
  return type[0] == '%';
},
  INT_TYPES: {"i1":0,"i8":0,"i16":0,"i32":0,"i64":0},
  FLOAT_TYPES: {"float":0,"double":0},
  or64: function (x, y) {
    var l = (x | 0) | (y | 0);
    var h = (Math.round(x / 4294967296) | Math.round(y / 4294967296)) * 4294967296;
    return l + h;
  },
  and64: function (x, y) {
    var l = (x | 0) & (y | 0);
    var h = (Math.round(x / 4294967296) & Math.round(y / 4294967296)) * 4294967296;
    return l + h;
  },
  xor64: function (x, y) {
    var l = (x | 0) ^ (y | 0);
    var h = (Math.round(x / 4294967296) ^ Math.round(y / 4294967296)) * 4294967296;
    return l + h;
  },
  getNativeTypeSize: function (type) {
    switch (type) {
      case 'i1': case 'i8': return 1;
      case 'i16': return 2;
      case 'i32': return 4;
      case 'i64': return 8;
      case 'float': return 4;
      case 'double': return 8;
      default: {
        if (type[type.length-1] === '*') {
          return Runtime.QUANTUM_SIZE; // A pointer
        } else if (type[0] === 'i') {
          var bits = parseInt(type.substr(1));
          assert(bits % 8 === 0);
          return bits/8;
        } else {
          return 0;
        }
      }
    }
  },
  getNativeFieldSize: function (type) {
    return Math.max(Runtime.getNativeTypeSize(type), Runtime.QUANTUM_SIZE);
  },
  dedup: function dedup(items, ident) {
  var seen = {};
  if (ident) {
    return items.filter(function(item) {
      if (seen[item[ident]]) return false;
      seen[item[ident]] = true;
      return true;
    });
  } else {
    return items.filter(function(item) {
      if (seen[item]) return false;
      seen[item] = true;
      return true;
    });
  }
},
  set: function set() {
  var args = typeof arguments[0] === 'object' ? arguments[0] : arguments;
  var ret = {};
  for (var i = 0; i < args.length; i++) {
    ret[args[i]] = 0;
  }
  return ret;
},
  STACK_ALIGN: 8,
  getAlignSize: function (type, size, vararg) {
    // we align i64s and doubles on 64-bit boundaries, unlike x86
    if (!vararg && (type == 'i64' || type == 'double')) return 8;
    if (!type) return Math.min(size, 8); // align structures internally to 64 bits
    return Math.min(size || (type ? Runtime.getNativeFieldSize(type) : 0), Runtime.QUANTUM_SIZE);
  },
  calculateStructAlignment: function calculateStructAlignment(type) {
    type.flatSize = 0;
    type.alignSize = 0;
    var diffs = [];
    var prev = -1;
    var index = 0;
    type.flatIndexes = type.fields.map(function(field) {
      index++;
      var size, alignSize;
      if (Runtime.isNumberType(field) || Runtime.isPointerType(field)) {
        size = Runtime.getNativeTypeSize(field); // pack char; char; in structs, also char[X]s.
        alignSize = Runtime.getAlignSize(field, size);
      } else if (Runtime.isStructType(field)) {
        if (field[1] === '0') {
          // this is [0 x something]. When inside another structure like here, it must be at the end,
          // and it adds no size
          // XXX this happens in java-nbody for example... assert(index === type.fields.length, 'zero-length in the middle!');
          size = 0;
          if (Types.types[field]) {
            alignSize = Runtime.getAlignSize(null, Types.types[field].alignSize);
          } else {
            alignSize = type.alignSize || QUANTUM_SIZE;
          }
        } else {
          size = Types.types[field].flatSize;
          alignSize = Runtime.getAlignSize(null, Types.types[field].alignSize);
        }
      } else if (field[0] == 'b') {
        // bN, large number field, like a [N x i8]
        size = field.substr(1)|0;
        alignSize = 1;
      } else if (field[0] === '<') {
        // vector type
        size = alignSize = Types.types[field].flatSize; // fully aligned
      } else if (field[0] === 'i') {
        // illegal integer field, that could not be legalized because it is an internal structure field
        // it is ok to have such fields, if we just use them as markers of field size and nothing more complex
        size = alignSize = parseInt(field.substr(1))/8;
        assert(size % 1 === 0, 'cannot handle non-byte-size field ' + field);
      } else {
        assert(false, 'invalid type for calculateStructAlignment');
      }
      if (type.packed) alignSize = 1;
      type.alignSize = Math.max(type.alignSize, alignSize);
      var curr = Runtime.alignMemory(type.flatSize, alignSize); // if necessary, place this on aligned memory
      type.flatSize = curr + size;
      if (prev >= 0) {
        diffs.push(curr-prev);
      }
      prev = curr;
      return curr;
    });
    if (type.name_ && type.name_[0] === '[') {
      // arrays have 2 elements, so we get the proper difference. then we scale here. that way we avoid
      // allocating a potentially huge array for [999999 x i8] etc.
      type.flatSize = parseInt(type.name_.substr(1))*type.flatSize/2;
    }
    type.flatSize = Runtime.alignMemory(type.flatSize, type.alignSize);
    if (diffs.length == 0) {
      type.flatFactor = type.flatSize;
    } else if (Runtime.dedup(diffs).length == 1) {
      type.flatFactor = diffs[0];
    }
    type.needsFlattening = (type.flatFactor != 1);
    return type.flatIndexes;
  },
  generateStructInfo: function (struct, typeName, offset) {
    var type, alignment;
    if (typeName) {
      offset = offset || 0;
      type = (typeof Types === 'undefined' ? Runtime.typeInfo : Types.types)[typeName];
      if (!type) return null;
      if (type.fields.length != struct.length) {
        printErr('Number of named fields must match the type for ' + typeName + ': possibly duplicate struct names. Cannot return structInfo');
        return null;
      }
      alignment = type.flatIndexes;
    } else {
      var type = { fields: struct.map(function(item) { return item[0] }) };
      alignment = Runtime.calculateStructAlignment(type);
    }
    var ret = {
      __size__: type.flatSize
    };
    if (typeName) {
      struct.forEach(function(item, i) {
        if (typeof item === 'string') {
          ret[item] = alignment[i] + offset;
        } else {
          // embedded struct
          var key;
          for (var k in item) key = k;
          ret[key] = Runtime.generateStructInfo(item[key], type.fields[i], alignment[i]);
        }
      });
    } else {
      struct.forEach(function(item, i) {
        ret[item[1]] = alignment[i];
      });
    }
    return ret;
  },
  dynCall: function (sig, ptr, args) {
    if (args && args.length) {
      assert(args.length == sig.length-1);
      if (!args.splice) args = Array.prototype.slice.call(args);
      args.splice(0, 0, ptr);
      assert(('dynCall_' + sig) in Module, 'bad function pointer type - no table for sig \'' + sig + '\'');
      return Module['dynCall_' + sig].apply(null, args);
    } else {
      assert(sig.length == 1);
      assert(('dynCall_' + sig) in Module, 'bad function pointer type - no table for sig \'' + sig + '\'');
      return Module['dynCall_' + sig].call(null, ptr);
    }
  },
  functionPointers: [],
  addFunction: function (func) {
    for (var i = 0; i < Runtime.functionPointers.length; i++) {
      if (!Runtime.functionPointers[i]) {
        Runtime.functionPointers[i] = func;
        return 2*(1 + i);
      }
    }
    throw 'Finished up all reserved function pointers. Use a higher value for RESERVED_FUNCTION_POINTERS.';
  },
  removeFunction: function (index) {
    Runtime.functionPointers[(index-2)/2] = null;
  },
  getAsmConst: function (code, numArgs) {
    // code is a constant string on the heap, so we can cache these
    if (!Runtime.asmConstCache) Runtime.asmConstCache = {};
    var func = Runtime.asmConstCache[code];
    if (func) return func;
    var args = [];
    for (var i = 0; i < numArgs; i++) {
      args.push(String.fromCharCode(36) + i); // $0, $1 etc
    }
    var source = Pointer_stringify(code);
    if (source[0] === '"') {
      // tolerate EM_ASM("..code..") even though EM_ASM(..code..) is correct
      if (source.indexOf('"', 1) === source.length-1) {
        source = source.substr(1, source.length-2);
      } else {
        // something invalid happened, e.g. EM_ASM("..code($0)..", input)
        abort('invalid EM_ASM input |' + source + '|. Please use EM_ASM(..code..) (no quotes) or EM_ASM({ ..code($0).. }, input) (to input values)');
      }
    }
    try {
      var evalled = eval('(function(' + args.join(',') + '){ ' + source + ' })'); // new Function does not allow upvars in node
    } catch(e) {
      Module.printErr('error in executing inline EM_ASM code: ' + e + ' on: \n\n' + source + '\n\nwith args |' + args + '| (make sure to use the right one out of EM_ASM, EM_ASM_ARGS, etc.)');
      throw e;
    }
    return Runtime.asmConstCache[code] = evalled;
  },
  warnOnce: function (text) {
    if (!Runtime.warnOnce.shown) Runtime.warnOnce.shown = {};
    if (!Runtime.warnOnce.shown[text]) {
      Runtime.warnOnce.shown[text] = 1;
      Module.printErr(text);
    }
  },
  funcWrappers: {},
  getFuncWrapper: function (func, sig) {
    assert(sig);
    if (!Runtime.funcWrappers[func]) {
      Runtime.funcWrappers[func] = function dynCall_wrapper() {
        return Runtime.dynCall(sig, func, arguments);
      };
    }
    return Runtime.funcWrappers[func];
  },
  UTF8Processor: function () {
    var buffer = [];
    var needed = 0;
    this.processCChar = function (code) {
      code = code & 0xFF;

      if (buffer.length == 0) {
        if ((code & 0x80) == 0x00) {        // 0xxxxxxx
          return String.fromCharCode(code);
        }
        buffer.push(code);
        if ((code & 0xE0) == 0xC0) {        // 110xxxxx
          needed = 1;
        } else if ((code & 0xF0) == 0xE0) { // 1110xxxx
          needed = 2;
        } else {                            // 11110xxx
          needed = 3;
        }
        return '';
      }

      if (needed) {
        buffer.push(code);
        needed--;
        if (needed > 0) return '';
      }

      var c1 = buffer[0];
      var c2 = buffer[1];
      var c3 = buffer[2];
      var c4 = buffer[3];
      var ret;
      if (buffer.length == 2) {
        ret = String.fromCharCode(((c1 & 0x1F) << 6)  | (c2 & 0x3F));
      } else if (buffer.length == 3) {
        ret = String.fromCharCode(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6)  | (c3 & 0x3F));
      } else {
        // http://mathiasbynens.be/notes/javascript-encoding#surrogate-formulae
        var codePoint = ((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) |
                        ((c3 & 0x3F) << 6)  | (c4 & 0x3F);
        ret = String.fromCharCode(
          Math.floor((codePoint - 0x10000) / 0x400) + 0xD800,
          (codePoint - 0x10000) % 0x400 + 0xDC00);
      }
      buffer.length = 0;
      return ret;
    }
    this.processJSString = function processJSString(string) {
      /* TODO: use TextEncoder when present,
        var encoder = new TextEncoder();
        encoder['encoding'] = "utf-8";
        var utf8Array = encoder['encode'](aMsg.data);
      */
      string = unescape(encodeURIComponent(string));
      var ret = [];
      for (var i = 0; i < string.length; i++) {
        ret.push(string.charCodeAt(i));
      }
      return ret;
    }
  },
  getCompilerSetting: function (name) {
    throw 'You must build with -s RETAIN_COMPILER_SETTINGS=1 for Runtime.getCompilerSetting or emscripten_get_compiler_setting to work';
  },
  stackAlloc: function (size) { var ret = STACKTOP;STACKTOP = (STACKTOP + size)|0;STACKTOP = (((STACKTOP)+7)&-8);(assert((((STACKTOP|0) < (STACK_MAX|0))|0))|0); return ret; },
  staticAlloc: function (size) { var ret = STATICTOP;STATICTOP = (STATICTOP + (assert(!staticSealed),size))|0;STATICTOP = (((STATICTOP)+7)&-8); return ret; },
  dynamicAlloc: function (size) { var ret = DYNAMICTOP;DYNAMICTOP = (DYNAMICTOP + (assert(DYNAMICTOP > 0),size))|0;DYNAMICTOP = (((DYNAMICTOP)+7)&-8); if (DYNAMICTOP >= TOTAL_MEMORY) enlargeMemory();; return ret; },
  alignMemory: function (size,quantum) { var ret = size = Math.ceil((size)/(quantum ? quantum : 8))*(quantum ? quantum : 8); return ret; },
  makeBigInt: function (low,high,unsigned) { var ret = (unsigned ? ((+((low>>>0)))+((+((high>>>0)))*4294967296.0)) : ((+((low>>>0)))+((+((high|0)))*4294967296.0))); return ret; },
  GLOBAL_BASE: 8,
  QUANTUM_SIZE: 4,
  __dummy__: 0
}


Module['Runtime'] = Runtime;









//========================================
// Runtime essentials
//========================================

var __THREW__ = 0; // Used in checking for thrown exceptions.

var ABORT = false; // whether we are quitting the application. no code should run after this. set in exit() and abort()
var EXITSTATUS = 0;

var undef = 0;
// tempInt is used for 32-bit signed values or smaller. tempBigInt is used
// for 32-bit unsigned values or more than 32 bits. TODO: audit all uses of tempInt
var tempValue, tempInt, tempBigInt, tempInt2, tempBigInt2, tempPair, tempBigIntI, tempBigIntR, tempBigIntS, tempBigIntP, tempBigIntD, tempDouble, tempFloat;
var tempI64, tempI64b;
var tempRet0, tempRet1, tempRet2, tempRet3, tempRet4, tempRet5, tempRet6, tempRet7, tempRet8, tempRet9;

function assert(condition, text) {
  if (!condition) {
    abort('Assertion failed: ' + text);
  }
}

var globalScope = this;

// C calling interface. A convenient way to call C functions (in C files, or
// defined with extern "C").
//
// Note: LLVM optimizations can inline and remove functions, after which you will not be
//       able to call them. Closure can also do so. To avoid that, add your function to
//       the exports using something like
//
//         -s EXPORTED_FUNCTIONS='["_main", "_myfunc"]'
//
// @param ident      The name of the C function (note that C++ functions will be name-mangled - use extern "C")
// @param returnType The return type of the function, one of the JS types 'number', 'string' or 'array' (use 'number' for any C pointer, and
//                   'array' for JavaScript arrays and typed arrays; note that arrays are 8-bit).
// @param argTypes   An array of the types of arguments for the function (if there are no arguments, this can be ommitted). Types are as in returnType,
//                   except that 'array' is not possible (there is no way for us to know the length of the array)
// @param args       An array of the arguments to the function, as native JS values (as in returnType)
//                   Note that string arguments will be stored on the stack (the JS string will become a C string on the stack).
// @return           The return value, as a native JS value (as in returnType)
function ccall(ident, returnType, argTypes, args) {
  return ccallFunc(getCFunc(ident), returnType, argTypes, args);
}
Module["ccall"] = ccall;

// Returns the C function with a specified identifier (for C++, you need to do manual name mangling)
function getCFunc(ident) {
  try {
    var func = Module['_' + ident]; // closure exported function
    if (!func) func = eval('_' + ident); // explicit lookup
  } catch(e) {
  }
  assert(func, 'Cannot call unknown function ' + ident + ' (perhaps LLVM optimizations or closure removed it?)');
  return func;
}

// Internal function that does a C call using a function, not an identifier
function ccallFunc(func, returnType, argTypes, args) {
  var stack = 0;
  function toC(value, type) {
    if (type == 'string') {
      if (value === null || value === undefined || value === 0) return 0; // null string
      value = intArrayFromString(value);
      type = 'array';
    }
    if (type == 'array') {
      if (!stack) stack = Runtime.stackSave();
      var ret = Runtime.stackAlloc(value.length);
      writeArrayToMemory(value, ret);
      return ret;
    }
    return value;
  }
  function fromC(value, type) {
    if (type == 'string') {
      return Pointer_stringify(value);
    }
    assert(type != 'array');
    return value;
  }
  var i = 0;
  var cArgs = args ? args.map(function(arg) {
    return toC(arg, argTypes[i++]);
  }) : [];
  var ret = fromC(func.apply(null, cArgs), returnType);
  if (stack) Runtime.stackRestore(stack);
  return ret;
}

// Returns a native JS wrapper for a C function. This is similar to ccall, but
// returns a function you can call repeatedly in a normal way. For example:
//
//   var my_function = cwrap('my_c_function', 'number', ['number', 'number']);
//   alert(my_function(5, 22));
//   alert(my_function(99, 12));
//
function cwrap(ident, returnType, argTypes) {
  var func = getCFunc(ident);
  return function() {
    return ccallFunc(func, returnType, argTypes, Array.prototype.slice.call(arguments));
  }
}
Module["cwrap"] = cwrap;

// Sets a value in memory in a dynamic way at run-time. Uses the
// type data. This is the same as makeSetValue, except that
// makeSetValue is done at compile-time and generates the needed
// code then, whereas this function picks the right code at
// run-time.
// Note that setValue and getValue only do *aligned* writes and reads!
// Note that ccall uses JS types as for defining types, while setValue and
// getValue need LLVM types ('i8', 'i32') - this is a lower-level operation
function setValue(ptr, value, type, noSafe) {
  type = type || 'i8';
  if (type.charAt(type.length-1) === '*') type = 'i32'; // pointers are 32-bit
    switch(type) {
      case 'i1': HEAP8[(ptr)]=value; break;
      case 'i8': HEAP8[(ptr)]=value; break;
      case 'i16': HEAP16[((ptr)>>1)]=value; break;
      case 'i32': HEAP32[((ptr)>>2)]=value; break;
      case 'i64': (tempI64 = [value>>>0,(tempDouble=value,(+(Math_abs(tempDouble))) >= 1.0 ? (tempDouble > 0.0 ? ((Math_min((+(Math_floor((tempDouble)/4294967296.0))), 4294967295.0))|0)>>>0 : (~~((+(Math_ceil((tempDouble - +(((~~(tempDouble)))>>>0))/4294967296.0)))))>>>0) : 0)],HEAP32[((ptr)>>2)]=tempI64[0],HEAP32[(((ptr)+(4))>>2)]=tempI64[1]); break;
      case 'float': HEAPF32[((ptr)>>2)]=value; break;
      case 'double': HEAPF64[((ptr)>>3)]=value; break;
      default: abort('invalid type for setValue: ' + type);
    }
}
Module['setValue'] = setValue;

// Parallel to setValue.
function getValue(ptr, type, noSafe) {
  type = type || 'i8';
  if (type.charAt(type.length-1) === '*') type = 'i32'; // pointers are 32-bit
    switch(type) {
      case 'i1': return HEAP8[(ptr)];
      case 'i8': return HEAP8[(ptr)];
      case 'i16': return HEAP16[((ptr)>>1)];
      case 'i32': return HEAP32[((ptr)>>2)];
      case 'i64': return HEAP32[((ptr)>>2)];
      case 'float': return HEAPF32[((ptr)>>2)];
      case 'double': return HEAPF64[((ptr)>>3)];
      default: abort('invalid type for setValue: ' + type);
    }
  return null;
}
Module['getValue'] = getValue;

var ALLOC_NORMAL = 0; // Tries to use _malloc()
var ALLOC_STACK = 1; // Lives for the duration of the current function call
var ALLOC_STATIC = 2; // Cannot be freed
var ALLOC_DYNAMIC = 3; // Cannot be freed except through sbrk
var ALLOC_NONE = 4; // Do not allocate
Module['ALLOC_NORMAL'] = ALLOC_NORMAL;
Module['ALLOC_STACK'] = ALLOC_STACK;
Module['ALLOC_STATIC'] = ALLOC_STATIC;
Module['ALLOC_DYNAMIC'] = ALLOC_DYNAMIC;
Module['ALLOC_NONE'] = ALLOC_NONE;

// allocate(): This is for internal use. You can use it yourself as well, but the interface
//             is a little tricky (see docs right below). The reason is that it is optimized
//             for multiple syntaxes to save space in generated code. So you should
//             normally not use allocate(), and instead allocate memory using _malloc(),
//             initialize it with setValue(), and so forth.
// @slab: An array of data, or a number. If a number, then the size of the block to allocate,
//        in *bytes* (note that this is sometimes confusing: the next parameter does not
//        affect this!)
// @types: Either an array of types, one for each byte (or 0 if no type at that position),
//         or a single type which is used for the entire block. This only matters if there
//         is initial data - if @slab is a number, then this does not matter at all and is
//         ignored.
// @allocator: How to allocate memory, see ALLOC_*
function allocate(slab, types, allocator, ptr) {
  var zeroinit, size;
  if (typeof slab === 'number') {
    zeroinit = true;
    size = slab;
  } else {
    zeroinit = false;
    size = slab.length;
  }

  var singleType = typeof types === 'string' ? types : null;

  var ret;
  if (allocator == ALLOC_NONE) {
    ret = ptr;
  } else {
    ret = [_malloc, Runtime.stackAlloc, Runtime.staticAlloc, Runtime.dynamicAlloc][allocator === undefined ? ALLOC_STATIC : allocator](Math.max(size, singleType ? 1 : types.length));
  }

  if (zeroinit) {
    var ptr = ret, stop;
    assert((ret & 3) == 0);
    stop = ret + (size & ~3);
    for (; ptr < stop; ptr += 4) {
      HEAP32[((ptr)>>2)]=0;
    }
    stop = ret + size;
    while (ptr < stop) {
      HEAP8[((ptr++)|0)]=0;
    }
    return ret;
  }

  if (singleType === 'i8') {
    if (slab.subarray || slab.slice) {
      HEAPU8.set(slab, ret);
    } else {
      HEAPU8.set(new Uint8Array(slab), ret);
    }
    return ret;
  }

  var i = 0, type, typeSize, previousType;
  while (i < size) {
    var curr = slab[i];

    if (typeof curr === 'function') {
      curr = Runtime.getFunctionIndex(curr);
    }

    type = singleType || types[i];
    if (type === 0) {
      i++;
      continue;
    }
    assert(type, 'Must know what type to store in allocate!');

    if (type == 'i64') type = 'i32'; // special case: we have one i32 here, and one i32 later

    setValue(ret+i, curr, type);

    // no need to look up size unless type changes, so cache it
    if (previousType !== type) {
      typeSize = Runtime.getNativeTypeSize(type);
      previousType = type;
    }
    i += typeSize;
  }

  return ret;
}
Module['allocate'] = allocate;

function Pointer_stringify(ptr, /* optional */ length) {
  // TODO: use TextDecoder
  // Find the length, and check for UTF while doing so
  var hasUtf = false;
  var t;
  var i = 0;
  while (1) {
    assert(ptr + i < TOTAL_MEMORY);
    t = HEAPU8[(((ptr)+(i))|0)];
    if (t >= 128) hasUtf = true;
    else if (t == 0 && !length) break;
    i++;
    if (length && i == length) break;
  }
  if (!length) length = i;

  var ret = '';

  if (!hasUtf) {
    var MAX_CHUNK = 1024; // split up into chunks, because .apply on a huge string can overflow the stack
    var curr;
    while (length > 0) {
      curr = String.fromCharCode.apply(String, HEAPU8.subarray(ptr, ptr + Math.min(length, MAX_CHUNK)));
      ret = ret ? ret + curr : curr;
      ptr += MAX_CHUNK;
      length -= MAX_CHUNK;
    }
    return ret;
  }

  var utf8 = new Runtime.UTF8Processor();
  for (i = 0; i < length; i++) {
    assert(ptr + i < TOTAL_MEMORY);
    t = HEAPU8[(((ptr)+(i))|0)];
    ret += utf8.processCChar(t);
  }
  return ret;
}
Module['Pointer_stringify'] = Pointer_stringify;

// Given a pointer 'ptr' to a null-terminated UTF16LE-encoded string in the emscripten HEAP, returns
// a copy of that string as a Javascript String object.
function UTF16ToString(ptr) {
  var i = 0;

  var str = '';
  while (1) {
    var codeUnit = HEAP16[(((ptr)+(i*2))>>1)];
    if (codeUnit == 0)
      return str;
    ++i;
    // fromCharCode constructs a character from a UTF-16 code unit, so we can pass the UTF16 string right through.
    str += String.fromCharCode(codeUnit);
  }
}
Module['UTF16ToString'] = UTF16ToString;

// Copies the given Javascript String object 'str' to the emscripten HEAP at address 'outPtr',
// null-terminated and encoded in UTF16LE form. The copy will require at most (str.length*2+1)*2 bytes of space in the HEAP.
function stringToUTF16(str, outPtr) {
  for(var i = 0; i < str.length; ++i) {
    // charCodeAt returns a UTF-16 encoded code unit, so it can be directly written to the HEAP.
    var codeUnit = str.charCodeAt(i); // possibly a lead surrogate
    HEAP16[(((outPtr)+(i*2))>>1)]=codeUnit;
  }
  // Null-terminate the pointer to the HEAP.
  HEAP16[(((outPtr)+(str.length*2))>>1)]=0;
}
Module['stringToUTF16'] = stringToUTF16;

// Given a pointer 'ptr' to a null-terminated UTF32LE-encoded string in the emscripten HEAP, returns
// a copy of that string as a Javascript String object.
function UTF32ToString(ptr) {
  var i = 0;

  var str = '';
  while (1) {
    var utf32 = HEAP32[(((ptr)+(i*4))>>2)];
    if (utf32 == 0)
      return str;
    ++i;
    // Gotcha: fromCharCode constructs a character from a UTF-16 encoded code (pair), not from a Unicode code point! So encode the code point to UTF-16 for constructing.
    if (utf32 >= 0x10000) {
      var ch = utf32 - 0x10000;
      str += String.fromCharCode(0xD800 | (ch >> 10), 0xDC00 | (ch & 0x3FF));
    } else {
      str += String.fromCharCode(utf32);
    }
  }
}
Module['UTF32ToString'] = UTF32ToString;

// Copies the given Javascript String object 'str' to the emscripten HEAP at address 'outPtr',
// null-terminated and encoded in UTF32LE form. The copy will require at most (str.length+1)*4 bytes of space in the HEAP,
// but can use less, since str.length does not return the number of characters in the string, but the number of UTF-16 code units in the string.
function stringToUTF32(str, outPtr) {
  var iChar = 0;
  for(var iCodeUnit = 0; iCodeUnit < str.length; ++iCodeUnit) {
    // Gotcha: charCodeAt returns a 16-bit word that is a UTF-16 encoded code unit, not a Unicode code point of the character! We must decode the string to UTF-32 to the heap.
    var codeUnit = str.charCodeAt(iCodeUnit); // possibly a lead surrogate
    if (codeUnit >= 0xD800 && codeUnit <= 0xDFFF) {
      var trailSurrogate = str.charCodeAt(++iCodeUnit);
      codeUnit = 0x10000 + ((codeUnit & 0x3FF) << 10) | (trailSurrogate & 0x3FF);
    }
    HEAP32[(((outPtr)+(iChar*4))>>2)]=codeUnit;
    ++iChar;
  }
  // Null-terminate the pointer to the HEAP.
  HEAP32[(((outPtr)+(iChar*4))>>2)]=0;
}
Module['stringToUTF32'] = stringToUTF32;

function demangle(func) {
  var i = 3;
  // params, etc.
  var basicTypes = {
    'v': 'void',
    'b': 'bool',
    'c': 'char',
    's': 'short',
    'i': 'int',
    'l': 'long',
    'f': 'float',
    'd': 'double',
    'w': 'wchar_t',
    'a': 'signed char',
    'h': 'unsigned char',
    't': 'unsigned short',
    'j': 'unsigned int',
    'm': 'unsigned long',
    'x': 'long long',
    'y': 'unsigned long long',
    'z': '...'
  };
  var subs = [];
  var first = true;
  function dump(x) {
    //return;
    if (x) Module.print(x);
    Module.print(func);
    var pre = '';
    for (var a = 0; a < i; a++) pre += ' ';
    Module.print (pre + '^');
  }
  function parseNested() {
    i++;
    if (func[i] === 'K') i++; // ignore const
    var parts = [];
    while (func[i] !== 'E') {
      if (func[i] === 'S') { // substitution
        i++;
        var next = func.indexOf('_', i);
        var num = func.substring(i, next) || 0;
        parts.push(subs[num] || '?');
        i = next+1;
        continue;
      }
      if (func[i] === 'C') { // constructor
        parts.push(parts[parts.length-1]);
        i += 2;
        continue;
      }
      var size = parseInt(func.substr(i));
      var pre = size.toString().length;
      if (!size || !pre) { i--; break; } // counter i++ below us
      var curr = func.substr(i + pre, size);
      parts.push(curr);
      subs.push(curr);
      i += pre + size;
    }
    i++; // skip E
    return parts;
  }
  function parse(rawList, limit, allowVoid) { // main parser
    limit = limit || Infinity;
    var ret = '', list = [];
    function flushList() {
      return '(' + list.join(', ') + ')';
    }
    var name;
    if (func[i] === 'N') {
      // namespaced N-E
      name = parseNested().join('::');
      limit--;
      if (limit === 0) return rawList ? [name] : name;
    } else {
      // not namespaced
      if (func[i] === 'K' || (first && func[i] === 'L')) i++; // ignore const and first 'L'
      var size = parseInt(func.substr(i));
      if (size) {
        var pre = size.toString().length;
        name = func.substr(i + pre, size);
        i += pre + size;
      }
    }
    first = false;
    if (func[i] === 'I') {
      i++;
      var iList = parse(true);
      var iRet = parse(true, 1, true);
      ret += iRet[0] + ' ' + name + '<' + iList.join(', ') + '>';
    } else {
      ret = name;
    }
    paramLoop: while (i < func.length && limit-- > 0) {
      //dump('paramLoop');
      var c = func[i++];
      if (c in basicTypes) {
        list.push(basicTypes[c]);
      } else {
        switch (c) {
          case 'P': list.push(parse(true, 1, true)[0] + '*'); break; // pointer
          case 'R': list.push(parse(true, 1, true)[0] + '&'); break; // reference
          case 'L': { // literal
            i++; // skip basic type
            var end = func.indexOf('E', i);
            var size = end - i;
            list.push(func.substr(i, size));
            i += size + 2; // size + 'EE'
            break;
          }
          case 'A': { // array
            var size = parseInt(func.substr(i));
            i += size.toString().length;
            if (func[i] !== '_') throw '?';
            i++; // skip _
            list.push(parse(true, 1, true)[0] + ' [' + size + ']');
            break;
          }
          case 'E': break paramLoop;
          default: ret += '?' + c; break paramLoop;
        }
      }
    }
    if (!allowVoid && list.length === 1 && list[0] === 'void') list = []; // avoid (void)
    if (rawList) {
      if (ret) {
        list.push(ret + '?');
      }
      return list;
    } else {
      return ret + flushList();
    }
  }
  try {
    // Special-case the entry point, since its name differs from other name mangling.
    if (func == 'Object._main' || func == '_main') {
      return 'main()';
    }
    if (typeof func === 'number') func = Pointer_stringify(func);
    if (func[0] !== '_') return func;
    if (func[1] !== '_') return func; // C function
    if (func[2] !== 'Z') return func;
    switch (func[3]) {
      case 'n': return 'operator new()';
      case 'd': return 'operator delete()';
    }
    return parse();
  } catch(e) {
    return func;
  }
}

function demangleAll(text) {
  return text.replace(/__Z[\w\d_]+/g, function(x) { var y = demangle(x); return x === y ? x : (x + ' [' + y + ']') });
}

function stackTrace() {
  var stack = new Error().stack;
  return stack ? demangleAll(stack) : '(no stack trace available)'; // Stack trace is not available at least on IE10 and Safari 6.
}

// Memory management

var PAGE_SIZE = 4096;
function alignMemoryPage(x) {
  return (x+4095)&-4096;
}

var HEAP;
var HEAP8, HEAPU8, HEAP16, HEAPU16, HEAP32, HEAPU32, HEAPF32, HEAPF64;

var STATIC_BASE = 0, STATICTOP = 0, staticSealed = false; // static area
var STACK_BASE = 0, STACKTOP = 0, STACK_MAX = 0; // stack area
var DYNAMIC_BASE = 0, DYNAMICTOP = 0; // dynamic area handled by sbrk

function enlargeMemory() {
  abort('Cannot enlarge memory arrays. Either (1) compile with -s TOTAL_MEMORY=X with X higher than the current value ' + TOTAL_MEMORY + ', (2) compile with ALLOW_MEMORY_GROWTH which adjusts the size at runtime but prevents some optimizations, or (3) set Module.TOTAL_MEMORY before the program runs.');
}

var TOTAL_STACK = Module['TOTAL_STACK'] || 5242880;
var TOTAL_MEMORY = Module['TOTAL_MEMORY'] || 16777216;
var FAST_MEMORY = Module['FAST_MEMORY'] || 2097152;

var totalMemory = 4096;
while (totalMemory < TOTAL_MEMORY || totalMemory < 2*TOTAL_STACK) {
  if (totalMemory < 16*1024*1024) {
    totalMemory *= 2;
  } else {
    totalMemory += 16*1024*1024
  }
}
if (totalMemory !== TOTAL_MEMORY) {
  Module.printErr('increasing TOTAL_MEMORY to ' + totalMemory + ' to be more reasonable');
  TOTAL_MEMORY = totalMemory;
}

// Initialize the runtime's memory
// check for full engine support (use string 'subarray' to avoid closure compiler confusion)
assert(typeof Int32Array !== 'undefined' && typeof Float64Array !== 'undefined' && !!(new Int32Array(1)['subarray']) && !!(new Int32Array(1)['set']),
       'JS engine does not provide full typed array support');

var buffer = new ArrayBuffer(TOTAL_MEMORY);
HEAP8 = new Int8Array(buffer);
HEAP16 = new Int16Array(buffer);
HEAP32 = new Int32Array(buffer);
HEAPU8 = new Uint8Array(buffer);
HEAPU16 = new Uint16Array(buffer);
HEAPU32 = new Uint32Array(buffer);
HEAPF32 = new Float32Array(buffer);
HEAPF64 = new Float64Array(buffer);

// Endianness check (note: assumes compiler arch was little-endian)
HEAP32[0] = 255;
assert(HEAPU8[0] === 255 && HEAPU8[3] === 0, 'Typed arrays 2 must be run on a little-endian system');

Module['HEAP'] = HEAP;
Module['HEAP8'] = HEAP8;
Module['HEAP16'] = HEAP16;
Module['HEAP32'] = HEAP32;
Module['HEAPU8'] = HEAPU8;
Module['HEAPU16'] = HEAPU16;
Module['HEAPU32'] = HEAPU32;
Module['HEAPF32'] = HEAPF32;
Module['HEAPF64'] = HEAPF64;

function callRuntimeCallbacks(callbacks) {
  while(callbacks.length > 0) {
    var callback = callbacks.shift();
    if (typeof callback == 'function') {
      callback();
      continue;
    }
    var func = callback.func;
    if (typeof func === 'number') {
      if (callback.arg === undefined) {
        Runtime.dynCall('v', func);
      } else {
        Runtime.dynCall('vi', func, [callback.arg]);
      }
    } else {
      func(callback.arg === undefined ? null : callback.arg);
    }
  }
}

var __ATPRERUN__  = []; // functions called before the runtime is initialized
var __ATINIT__    = []; // functions called during startup
var __ATMAIN__    = []; // functions called when main() is to be run
var __ATEXIT__    = []; // functions called during shutdown
var __ATPOSTRUN__ = []; // functions called after the runtime has exited

var runtimeInitialized = false;

function preRun() {
  // compatibility - merge in anything from Module['preRun'] at this time
  if (Module['preRun']) {
    if (typeof Module['preRun'] == 'function') Module['preRun'] = [Module['preRun']];
    while (Module['preRun'].length) {
      addOnPreRun(Module['preRun'].shift());
    }
  }
  callRuntimeCallbacks(__ATPRERUN__);
}

function ensureInitRuntime() {
  if (runtimeInitialized) return;
  runtimeInitialized = true;
  callRuntimeCallbacks(__ATINIT__);
}

function preMain() {
  callRuntimeCallbacks(__ATMAIN__);
}

function exitRuntime() {
  if (ENVIRONMENT_IS_WEB || ENVIRONMENT_IS_WORKER) {
    Module.printErr('Exiting runtime. Any attempt to access the compiled C code may fail from now. If you want to keep the runtime alive, set Module["noExitRuntime"] = true or build with -s NO_EXIT_RUNTIME=1');
  }
  callRuntimeCallbacks(__ATEXIT__);
}

function postRun() {
  // compatibility - merge in anything from Module['postRun'] at this time
  if (Module['postRun']) {
    if (typeof Module['postRun'] == 'function') Module['postRun'] = [Module['postRun']];
    while (Module['postRun'].length) {
      addOnPostRun(Module['postRun'].shift());
    }
  }
  callRuntimeCallbacks(__ATPOSTRUN__);
}

function addOnPreRun(cb) {
  __ATPRERUN__.unshift(cb);
}
Module['addOnPreRun'] = Module.addOnPreRun = addOnPreRun;

function addOnInit(cb) {
  __ATINIT__.unshift(cb);
}
Module['addOnInit'] = Module.addOnInit = addOnInit;

function addOnPreMain(cb) {
  __ATMAIN__.unshift(cb);
}
Module['addOnPreMain'] = Module.addOnPreMain = addOnPreMain;

function addOnExit(cb) {
  __ATEXIT__.unshift(cb);
}
Module['addOnExit'] = Module.addOnExit = addOnExit;

function addOnPostRun(cb) {
  __ATPOSTRUN__.unshift(cb);
}
Module['addOnPostRun'] = Module.addOnPostRun = addOnPostRun;

// Tools

// This processes a JS string into a C-line array of numbers, 0-terminated.
// For LLVM-originating strings, see parser.js:parseLLVMString function
function intArrayFromString(stringy, dontAddNull, length /* optional */) {
  var ret = (new Runtime.UTF8Processor()).processJSString(stringy);
  if (length) {
    ret.length = length;
  }
  if (!dontAddNull) {
    ret.push(0);
  }
  return ret;
}
Module['intArrayFromString'] = intArrayFromString;

function intArrayToString(array) {
  var ret = [];
  for (var i = 0; i < array.length; i++) {
    var chr = array[i];
    if (chr > 0xFF) {
        assert(false, 'Character code ' + chr + ' (' + String.fromCharCode(chr) + ')  at offset ' + i + ' not in 0x00-0xFF.');
      chr &= 0xFF;
    }
    ret.push(String.fromCharCode(chr));
  }
  return ret.join('');
}
Module['intArrayToString'] = intArrayToString;

// Write a Javascript array to somewhere in the heap
function writeStringToMemory(string, buffer, dontAddNull) {
  var array = intArrayFromString(string, dontAddNull);
  var i = 0;
  while (i < array.length) {
    var chr = array[i];
    HEAP8[(((buffer)+(i))|0)]=chr;
    i = i + 1;
  }
}
Module['writeStringToMemory'] = writeStringToMemory;

function writeArrayToMemory(array, buffer) {
  for (var i = 0; i < array.length; i++) {
    HEAP8[(((buffer)+(i))|0)]=array[i];
  }
}
Module['writeArrayToMemory'] = writeArrayToMemory;

function writeAsciiToMemory(str, buffer, dontAddNull) {
  for (var i = 0; i < str.length; i++) {
    assert(str.charCodeAt(i) === str.charCodeAt(i)&0xff);
    HEAP8[(((buffer)+(i))|0)]=str.charCodeAt(i);
  }
  if (!dontAddNull) HEAP8[(((buffer)+(str.length))|0)]=0;
}
Module['writeAsciiToMemory'] = writeAsciiToMemory;

function unSign(value, bits, ignore) {
  if (value >= 0) {
    return value;
  }
  return bits <= 32 ? 2*Math.abs(1 << (bits-1)) + value // Need some trickery, since if bits == 32, we are right at the limit of the bits JS uses in bitshifts
                    : Math.pow(2, bits)         + value;
}
function reSign(value, bits, ignore) {
  if (value <= 0) {
    return value;
  }
  var half = bits <= 32 ? Math.abs(1 << (bits-1)) // abs is needed if bits == 32
                        : Math.pow(2, bits-1);
  if (value >= half && (bits <= 32 || value > half)) { // for huge values, we can hit the precision limit and always get true here. so don't do that
                                                       // but, in general there is no perfect solution here. With 64-bit ints, we get rounding and errors
                                                       // TODO: In i64 mode 1, resign the two parts separately and safely
    value = -2*half + value; // Cannot bitshift half, as it may be at the limit of the bits JS uses in bitshifts
  }
  return value;
}

// check for imul support, and also for correctness ( https://bugs.webkit.org/show_bug.cgi?id=126345 )
if (!Math['imul'] || Math['imul'](0xffffffff, 5) !== -5) Math['imul'] = function imul(a, b) {
  var ah  = a >>> 16;
  var al = a & 0xffff;
  var bh  = b >>> 16;
  var bl = b & 0xffff;
  return (al*bl + ((ah*bl + al*bh) << 16))|0;
};
Math.imul = Math['imul'];


var Math_abs = Math.abs;
var Math_cos = Math.cos;
var Math_sin = Math.sin;
var Math_tan = Math.tan;
var Math_acos = Math.acos;
var Math_asin = Math.asin;
var Math_atan = Math.atan;
var Math_atan2 = Math.atan2;
var Math_exp = Math.exp;
var Math_log = Math.log;
var Math_sqrt = Math.sqrt;
var Math_ceil = Math.ceil;
var Math_floor = Math.floor;
var Math_pow = Math.pow;
var Math_imul = Math.imul;
var Math_fround = Math.fround;
var Math_min = Math.min;

// A counter of dependencies for calling run(). If we need to
// do asynchronous work before running, increment this and
// decrement it. Incrementing must happen in a place like
// PRE_RUN_ADDITIONS (used by emcc to add file preloading).
// Note that you can add dependencies in preRun, even though
// it happens right before run - run will be postponed until
// the dependencies are met.
var runDependencies = 0;
var runDependencyWatcher = null;
var dependenciesFulfilled = null; // overridden to take different actions when all run dependencies are fulfilled
var runDependencyTracking = {};

function addRunDependency(id) {
  runDependencies++;
  if (Module['monitorRunDependencies']) {
    Module['monitorRunDependencies'](runDependencies);
  }
  if (id) {
    assert(!runDependencyTracking[id]);
    runDependencyTracking[id] = 1;
    if (runDependencyWatcher === null && typeof setInterval !== 'undefined') {
      // Check for missing dependencies every few seconds
      runDependencyWatcher = setInterval(function() {
        var shown = false;
        for (var dep in runDependencyTracking) {
          if (!shown) {
            shown = true;
            Module.printErr('still waiting on run dependencies:');
          }
          Module.printErr('dependency: ' + dep);
        }
        if (shown) {
          Module.printErr('(end of list)');
        }
      }, 10000);
    }
  } else {
    Module.printErr('warning: run dependency added without ID');
  }
}
Module['addRunDependency'] = addRunDependency;
function removeRunDependency(id) {
  runDependencies--;
  if (Module['monitorRunDependencies']) {
    Module['monitorRunDependencies'](runDependencies);
  }
  if (id) {
    assert(runDependencyTracking[id]);
    delete runDependencyTracking[id];
  } else {
    Module.printErr('warning: run dependency removed without ID');
  }
  if (runDependencies == 0) {
    if (runDependencyWatcher !== null) {
      clearInterval(runDependencyWatcher);
      runDependencyWatcher = null;
    }
    if (dependenciesFulfilled) {
      var callback = dependenciesFulfilled;
      dependenciesFulfilled = null;
      callback(); // can add another dependenciesFulfilled
    }
  }
}
Module['removeRunDependency'] = removeRunDependency;

Module["preloadedImages"] = {}; // maps url to image data
Module["preloadedAudios"] = {}; // maps url to audio data


var memoryInitializer = null;

// === Body ===





STATIC_BASE = 8;

STATICTOP = STATIC_BASE + Runtime.alignMemory(11699);
/* global initializers */ __ATINIT__.push();


/* memory initializer */ allocate([49,46,50,46,56,0,0,0,3,0,4,0,5,0,6,0,7,0,8,0,9,0,10,0,11,0,13,0,15,0,17,0,19,0,23,0,27,0,31,0,35,0,43,0,51,0,59,0,67,0,83,0,99,0,115,0,131,0,163,0,195,0,227,0,2,1,0,0,0,0,0,0,16,0,16,0,16,0,16,0,16,0,16,0,16,0,16,0,17,0,17,0,17,0,17,0,18,0,18,0,18,0,18,0,19,0,19,0,19,0,19,0,20,0,20,0,20,0,20,0,21,0,21,0,21,0,21,0,16,0,72,0,78,0,0,0,1,0,2,0,3,0,4,0,5,0,7,0,9,0,13,0,17,0,25,0,33,0,49,0,65,0,97,0,129,0,193,0,1,1,129,1,1,2,1,3,1,4,1,6,1,8,1,12,1,16,1,24,1,32,1,48,1,64,1,96,0,0,0,0,16,0,16,0,16,0,16,0,17,0,17,0,18,0,18,0,19,0,19,0,20,0,20,0,21,0,21,0,22,0,22,0,23,0,23,0,24,0,24,0,25,0,25,0,26,0,26,0,27,0,27,0,28,0,28,0,29,0,29,0,64,0,64,0,0,0,0,0,150,48,7,119,44,97,14,238,186,81,9,153,25,196,109,7,143,244,106,112,53,165,99,233,163,149,100,158,50,136,219,14,164,184,220,121,30,233,213,224,136,217,210,151,43,76,182,9,189,124,177,126,7,45,184,231,145,29,191,144,100,16,183,29,242,32,176,106,72,113,185,243,222,65,190,132,125,212,218,26,235,228,221,109,81,181,212,244,199,133,211,131,86,152,108,19,192,168,107,100,122,249,98,253,236,201,101,138,79,92,1,20,217,108,6,99,99,61,15,250,245,13,8,141,200,32,110,59,94,16,105,76,228,65,96,213,114,113,103,162,209,228,3,60,71,212,4,75,253,133,13,210,107,181,10,165,250,168,181,53,108,152,178,66,214,201,187,219,64,249,188,172,227,108,216,50,117,92,223,69,207,13,214,220,89,61,209,171,172,48,217,38,58,0,222,81,128,81,215,200,22,97,208,191,181,244,180,33,35,196,179,86,153,149,186,207,15,165,189,184,158,184,2,40,8,136,5,95,178,217,12,198,36,233,11,177,135,124,111,47,17,76,104,88,171,29,97,193,61,45,102,182,144,65,220,118,6,113,219,1,188,32,210,152,42,16,213,239,137,133,177,113,31,181,182,6,165,228,191,159,51,212,184,232,162,201,7,120,52,249,0,15,142,168,9,150,24,152,14,225,187,13,106,127,45,61,109,8,151,108,100,145,1,92,99,230,244,81,107,107,98,97,108,28,216,48,101,133,78,0,98,242,237,149,6,108,123,165,1,27,193,244,8,130,87,196,15,245,198,217,176,101,80,233,183,18,234,184,190,139,124,136,185,252,223,29,221,98,73,45,218,21,243,124,211,140,101,76,212,251,88,97,178,77,206,81,181,58,116,0,188,163,226,48,187,212,65,165,223,74,215,149,216,61,109,196,209,164,251,244,214,211,106,233,105,67,252,217,110,52,70,136,103,173,208,184,96,218,115,45,4,68,229,29,3,51,95,76,10,170,201,124,13,221,60,113,5,80,170,65,2,39,16,16,11,190,134,32,12,201,37,181,104,87,179,133,111,32,9,212,102,185,159,228,97,206,14,249,222,94,152,201,217,41,34,152,208,176,180,168,215,199,23,61,179,89,129,13,180,46,59,92,189,183,173,108,186,192,32,131,184,237,182,179,191,154,12,226,182,3,154,210,177,116,57,71,213,234,175,119,210,157,21,38,219,4,131,22,220,115,18,11,99,227,132,59,100,148,62,106,109,13,168,90,106,122,11,207,14,228,157,255,9,147,39,174,0,10,177,158,7,125,68,147,15,240,210,163,8,135,104,242,1,30,254,194,6,105,93,87,98,247,203,103,101,128,113,54,108,25,231,6,107,110,118,27,212,254,224,43,211,137,90,122,218,16,204,74,221,103,111,223,185,249,249,239,190,142,67,190,183,23,213,142,176,96,232,163,214,214,126,147,209,161,196,194,216,56,82,242,223,79,241,103,187,209,103,87,188,166,221,6,181,63,75,54,178,72,218,43,13,216,76,27,10,175,246,74,3,54,96,122,4,65,195,239,96,223,85,223,103,168,239,142,110,49,121,190,105,70,140,179,97,203,26,131,102,188,160,210,111,37,54,226,104,82,149,119,12,204,3,71,11,187,185,22,2,34,47,38,5,85,190,59,186,197,40,11,189,178,146,90,180,43,4,106,179,92,167,255,215,194,49,207,208,181,139,158,217,44,29,174,222,91,176,194,100,155,38,242,99,236,156,163,106,117,10,147,109,2,169,6,9,156,63,54,14,235,133,103,7,114,19,87,0,5,130,74,191,149,20,122,184,226,174,43,177,123,56,27,182,12,155,142,210,146,13,190,213,229,183,239,220,124,33,223,219,11,212,210,211,134,66,226,212,241,248,179,221,104,110,131,218,31,205,22,190,129,91,38,185,246,225,119,176,111,119,71,183,24,230,90,8,136,112,106,15,255,202,59,6,102,92,11,1,17,255,158,101,143,105,174,98,248,211,255,107,97,69,207,108,22,120,226,10,160,238,210,13,215,84,131,4,78,194,179,3,57,97,38,103,167,247,22,96,208,77,71,105,73,219,119,110,62,74,106,209,174,220,90,214,217,102,11,223,64,240,59,216,55,83,174,188,169,197,158,187,222,127,207,178,71,233,255,181,48,28,242,189,189,138,194,186,202,48,147,179,83,166,163,180,36,5,54,208,186,147,6,215,205,41,87,222,84,191,103,217,35,46,122,102,179,184,74,97,196,2,27,104,93,148,43,111,42,55,190,11,180,161,142,12,195,27,223,5,90,141,239,2,45], "i8", ALLOC_NONE, Runtime.GLOBAL_BASE);
/* memory initializer */ allocate([105,110,118,97,108,105,100,32,100,105,115,116,97,110,99,101,32,116,111,111,32,102,97,114,32,98,97,99,107,0,0,0,105,110,118,97,108,105,100,32,100,105,115,116,97,110,99,101,32,99,111,100,101,0,0,0,105,110,118,97,108,105,100,32,108,105,116,101,114,97,108,47,108,101,110,103,116,104,32,99,111,100,101,0,0,0,0,0,16,0,17,0,18,0,0,0,8,0,7,0,9,0,6,0,10,0,5,0,11,0,4,0,12,0,3,0,13,0,2,0,14,0,1,0,15,0,0,0,105,110,99,111,114,114,101,99,116,32,104,101,97,100,101,114,32,99,104,101,99,107,0,0,117,110,107,110,111,119,110,32,99,111,109,112,114,101,115,115,105,111,110,32,109,101,116,104,111,100,0,0,0,0,0,0,105,110,118,97,108,105,100,32,119,105,110,100,111,119,32,115,105,122,101,0,0,0,0,0,117,110,107,110,111,119,110,32,104,101,97,100,101,114,32,102,108,97,103,115,32,115,101,116,0,0,0,0,0,0,0,0,104,101,97,100,101,114,32,99,114,99,32,109,105,115,109,97,116,99,104,0,0,0,0,0,105,110,118,97,108,105,100,32,98,108,111,99,107,32,116,121,112,101,0,0,0,0,0,0,105,110,118,97,108,105,100,32,115,116,111,114,101,100,32,98,108,111,99,107,32,108,101,110,103,116,104,115,0,0,0,0,116,111,111,32,109,97,110,121,32,108,101,110,103,116,104,32,111,114,32,100,105,115,116,97,110,99,101,32,115,121,109,98,111,108,115,0,0,0,0,0,105,110,118,97,108,105,100,32,99,111,100,101,32,108,101,110,103,116,104,115,32,115,101,116,0,0,0,0,0,0,0,0,105,110,118,97,108,105,100,32,98,105,116,32,108,101,110,103,116,104,32,114,101,112,101,97,116,0,0,0,0,0,0,0,105,110,118,97,108,105,100,32,99,111,100,101,32,45,45,32,109,105,115,115,105,110,103,32,101,110,100,45,111,102,45,98,108,111,99,107,0,0,0,0,105,110,118,97,108,105,100,32,108,105,116,101,114,97,108,47,108,101,110,103,116,104,115,32,115,101,116,0,0,0,0,0,105,110,118,97,108,105,100,32,100,105,115,116,97,110,99,101,115,32,115,101,116,0,0,0,105,110,99,111,114,114,101,99,116,32,100,97,116,97,32,99,104,101,99,107,0,0,0,0,105,110,99,111,114,114,101,99,116,32,108,101,110,103,116,104,32,99,104,101,99,107,0,0,96,7,0,0,0,8,80,0,0,8,16,0,20,8,115,0,18,7,31,0,0,8,112,0,0,8,48,0,0,9,192,0,16,7,10,0,0,8,96,0,0,8,32,0,0,9,160,0,0,8,0,0,0,8,128,0,0,8,64,0,0,9,224,0,16,7,6,0,0,8,88,0,0,8,24,0,0,9,144,0,19,7,59,0,0,8,120,0,0,8,56,0,0,9,208,0,17,7,17,0,0,8,104,0,0,8,40,0,0,9,176,0,0,8,8,0,0,8,136,0,0,8,72,0,0,9,240,0,16,7,4,0,0,8,84,0,0,8,20,0,21,8,227,0,19,7,43,0,0,8,116,0,0,8,52,0,0,9,200,0,17,7,13,0,0,8,100,0,0,8,36,0,0,9,168,0,0,8,4,0,0,8,132,0,0,8,68,0,0,9,232,0,16,7,8,0,0,8,92,0,0,8,28,0,0,9,152,0,20,7,83,0,0,8,124,0,0,8,60,0,0,9,216,0,18,7,23,0,0,8,108,0,0,8,44,0,0,9,184,0,0,8,12,0,0,8,140,0,0,8,76,0,0,9,248,0,16,7,3,0,0,8,82,0,0,8,18,0,21,8,163,0,19,7,35,0,0,8,114,0,0,8,50,0,0,9,196,0,17,7,11,0,0,8,98,0,0,8,34,0,0,9,164,0,0,8,2,0,0,8,130,0,0,8,66,0,0,9,228,0,16,7,7,0,0,8,90,0,0,8,26,0,0,9,148,0,20,7,67,0,0,8,122,0,0,8,58,0,0,9,212,0,18,7,19,0,0,8,106,0,0,8,42,0,0,9,180,0,0,8,10,0,0,8,138,0,0,8,74,0,0,9,244,0,16,7,5,0,0,8,86,0,0,8,22,0,64,8,0,0,19,7,51,0,0,8,118,0,0,8,54,0,0,9,204,0,17,7,15,0,0,8,102,0,0,8,38,0,0,9,172,0,0,8,6,0,0,8,134,0,0,8,70,0,0,9,236,0,16,7,9,0,0,8,94,0,0,8,30,0,0,9,156,0,20,7,99,0,0,8,126,0,0,8,62,0,0,9,220,0,18,7,27,0,0,8,110,0,0,8,46,0,0,9,188,0,0,8,14,0,0,8,142,0,0,8,78,0,0,9,252,0,96,7,0,0,0,8,81,0,0,8,17,0,21,8,131,0,18,7,31,0,0,8,113,0,0,8,49,0,0,9,194,0,16,7,10,0,0,8,97,0,0,8,33,0,0,9,162,0,0,8,1,0,0,8,129,0,0,8,65,0,0,9,226,0,16,7,6,0,0,8,89,0,0,8,25,0,0,9,146,0,19,7,59,0,0,8,121,0,0,8,57,0,0,9,210,0,17,7,17,0,0,8,105,0,0,8,41,0,0,9,178,0,0,8,9,0,0,8,137,0,0,8,73,0,0,9,242,0,16,7,4,0,0,8,85,0,0,8,21,0,16,8,2,1,19,7,43,0,0,8,117,0,0,8,53,0,0,9,202,0,17,7,13,0,0,8,101,0,0,8,37,0,0,9,170,0,0,8,5,0,0,8,133,0,0,8,69,0,0,9,234,0,16,7,8,0,0,8,93,0,0,8,29,0,0,9,154,0,20,7,83,0,0,8,125,0,0,8,61,0,0,9,218,0,18,7,23,0,0,8,109,0,0,8,45,0,0,9,186,0,0,8,13,0,0,8,141,0,0,8,77,0,0,9,250,0,16,7,3,0,0,8,83,0,0,8,19,0,21,8,195,0,19,7,35,0,0,8,115,0,0,8,51,0,0,9,198,0,17,7,11,0,0,8,99,0,0,8,35,0,0,9,166,0,0,8,3,0,0,8,131,0,0,8,67,0,0,9,230,0,16,7,7,0,0,8,91,0,0,8,27,0,0,9,150,0,20,7,67,0,0,8,123,0,0,8,59,0,0,9,214,0,18,7,19,0,0,8,107,0,0,8,43,0,0,9,182,0,0,8,11,0,0,8,139,0,0,8,75,0,0,9,246,0,16,7,5,0,0,8,87,0,0,8,23,0,64,8,0,0,19,7,51,0,0,8,119,0,0,8,55,0,0,9,206,0,17,7,15,0,0,8,103,0,0,8,39,0,0,9,174,0,0,8,7,0,0,8,135,0,0,8,71,0,0,9,238,0,16,7,9,0,0,8,95,0,0,8,31,0,0,9,158,0,20,7,99,0,0,8,127,0,0,8,63,0,0,9,222,0,18,7,27,0,0,8,111,0,0,8,47,0,0,9,190,0,0,8,15,0,0,8,143,0,0,8,79,0,0,9,254,0,96,7,0,0,0,8,80,0,0,8,16,0,20,8,115,0,18,7,31,0,0,8,112,0,0,8,48,0,0,9,193,0,16,7,10,0,0,8,96,0,0,8,32,0,0,9,161,0,0,8,0,0,0,8,128,0,0,8,64,0,0,9,225,0,16,7,6,0,0,8,88,0,0,8,24,0,0,9,145,0,19,7,59,0,0,8,120,0,0,8,56,0,0,9,209,0,17,7,17,0,0,8,104,0,0,8,40,0,0,9,177,0,0,8,8,0,0,8,136,0,0,8,72,0,0,9,241,0,16,7,4,0,0,8,84,0,0,8,20,0,21,8,227,0,19,7,43,0,0,8,116,0,0,8,52,0,0,9,201,0,17,7,13,0,0,8,100,0,0,8,36,0,0,9,169,0,0,8,4,0,0,8,132,0,0,8,68,0,0,9,233,0,16,7,8,0,0,8,92,0,0,8,28,0,0,9,153,0,20,7,83,0,0,8,124,0,0,8,60,0,0,9,217,0,18,7,23,0,0,8,108,0,0,8,44,0,0,9,185,0,0,8,12,0,0,8,140,0,0,8,76,0,0,9,249,0,16,7,3,0,0,8,82,0,0,8,18,0,21,8,163,0,19,7,35,0,0,8,114,0,0,8,50,0,0,9,197,0,17,7,11,0,0,8,98,0,0,8,34,0,0,9,165,0,0,8,2,0,0,8,130,0,0,8,66,0,0,9,229,0,16,7,7,0,0,8,90,0,0,8,26,0,0,9,149,0,20,7,67,0,0,8,122,0,0,8,58,0,0,9,213,0,18,7,19,0,0,8,106,0,0,8,42,0,0,9,181,0,0,8,10,0,0,8,138,0,0,8,74,0,0,9,245,0,16,7,5,0,0,8,86,0,0,8,22,0,64,8,0,0,19,7,51,0,0,8,118,0,0,8,54,0,0,9,205,0,17,7,15,0,0,8,102,0,0,8,38,0,0,9,173,0,0,8,6,0,0,8,134,0,0,8,70,0,0,9,237,0,16,7,9,0,0,8,94,0,0,8,30,0,0,9,157,0,20,7,99,0,0,8,126,0,0,8,62,0,0,9,221,0,18,7,27,0,0,8,110,0,0,8,46,0,0,9,189,0,0,8,14,0,0,8,142,0,0,8,78,0,0,9,253,0,96,7,0,0,0,8,81,0,0,8,17,0,21,8,131,0,18,7,31,0,0,8,113,0,0,8,49,0,0,9,195,0,16,7,10,0,0,8,97,0,0,8,33,0,0,9,163,0,0,8,1,0,0,8,129,0,0,8,65,0,0,9,227,0,16,7,6,0,0,8,89,0,0,8,25,0,0,9,147,0,19,7,59,0,0,8,121,0,0,8,57,0,0,9,211,0,17,7,17,0,0,8,105,0,0,8,41,0,0,9,179,0,0,8,9,0,0,8,137,0,0,8,73,0,0,9,243,0,16,7,4,0,0,8,85,0,0,8,21,0,16,8,2,1,19,7,43,0,0,8,117,0,0,8,53,0,0,9,203,0,17,7,13,0,0,8,101,0,0,8,37,0,0,9,171,0,0,8,5,0,0,8,133,0,0,8,69,0,0,9,235,0,16,7,8,0,0,8,93,0,0,8,29,0,0,9,155,0,20,7,83,0,0,8,125,0,0,8,61,0,0,9,219,0,18,7,23,0,0,8,109,0,0,8,45,0,0,9,187,0,0,8,13,0,0,8,141,0,0,8,77,0,0,9,251,0,16,7,3,0,0,8,83,0,0,8,19,0,21,8,195,0,19,7,35,0,0,8,115,0,0,8,51,0,0,9,199,0,17,7,11,0,0,8,99,0,0,8,35,0,0,9,167,0,0,8,3,0,0,8,131,0,0,8,67,0,0,9,231,0,16,7,7,0,0,8,91,0,0,8,27,0,0,9,151,0,20,7,67,0,0,8,123,0,0,8,59,0,0,9,215,0,18,7,19,0,0,8,107,0,0,8,43,0,0,9,183,0,0,8,11,0,0,8,139,0,0,8,75,0,0,9,247,0,16,7,5,0,0,8,87,0,0,8,23,0,64,8,0,0,19,7,51,0,0,8,119,0,0,8,55,0,0,9,207,0,17,7,15,0,0,8,103,0,0,8,39,0,0,9,175,0,0,8,7,0,0,8,135,0,0,8,71,0,0,9,239,0,16,7,9,0,0,8,95,0,0,8,31,0,0,9,159,0,20,7,99,0,0,8,127,0,0,8,63,0,0,9,223,0,18,7,27,0,0,8,111,0,0,8,47,0,0,9,191,0,0,8,15,0,0,8,143,0,0,8,79,0,0,9,255,0,16,5,1,0,23,5,1,1,19,5,17,0,27,5,1,16,17,5,5,0,25,5,1,4,21,5,65,0,29,5,1,64,16,5,3,0,24,5,1,2,20,5,33,0,28,5,1,32,18,5,9,0,26,5,1,8,22,5,129,0,64,5,0,0,16,5,2,0,23,5,129,1,19,5,25,0,27,5,1,24,17,5,7,0,25,5,1,6,21,5,97,0,29,5,1,96,16,5,4,0,24,5,1,3,20,5,49,0,28,5,1,48,18,5,13,0,26,5,1,12,22,5,193,0,64,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "i8", ALLOC_NONE, Runtime.GLOBAL_BASE+8456);




var tempDoublePtr = Runtime.alignMemory(allocate(12, "i8", ALLOC_STATIC), 8);

assert(tempDoublePtr % 8 == 0);

function copyTempFloat(ptr) { // functions, because inlining this code increases code size too much

  HEAP8[tempDoublePtr] = HEAP8[ptr];

  HEAP8[tempDoublePtr+1] = HEAP8[ptr+1];

  HEAP8[tempDoublePtr+2] = HEAP8[ptr+2];

  HEAP8[tempDoublePtr+3] = HEAP8[ptr+3];

}

function copyTempDouble(ptr) {

  HEAP8[tempDoublePtr] = HEAP8[ptr];

  HEAP8[tempDoublePtr+1] = HEAP8[ptr+1];

  HEAP8[tempDoublePtr+2] = HEAP8[ptr+2];

  HEAP8[tempDoublePtr+3] = HEAP8[ptr+3];

  HEAP8[tempDoublePtr+4] = HEAP8[ptr+4];

  HEAP8[tempDoublePtr+5] = HEAP8[ptr+5];

  HEAP8[tempDoublePtr+6] = HEAP8[ptr+6];

  HEAP8[tempDoublePtr+7] = HEAP8[ptr+7];

}


  function _sbrk(bytes) {
      // Implement a Linux-like 'memory area' for our 'process'.
      // Changes the size of the memory area by |bytes|; returns the
      // address of the previous top ('break') of the memory area
      // We control the "dynamic" memory - DYNAMIC_BASE to DYNAMICTOP
      var self = _sbrk;
      if (!self.called) {
        DYNAMICTOP = alignMemoryPage(DYNAMICTOP); // make sure we start out aligned
        self.called = true;
        assert(Runtime.dynamicAlloc);
        self.alloc = Runtime.dynamicAlloc;
        Runtime.dynamicAlloc = function() { abort('cannot dynamically allocate, sbrk now has control') };
      }
      var ret = DYNAMICTOP;
      if (bytes != 0) self.alloc(bytes);
      return ret;  // Previous break location.
    }

  
  
  var ___errno_state=0;function ___setErrNo(value) {
      // For convenient setting and returning of errno.
      HEAP32[((___errno_state)>>2)]=value;
      return value;
    }
  
  var ERRNO_CODES={EPERM:1,ENOENT:2,ESRCH:3,EINTR:4,EIO:5,ENXIO:6,E2BIG:7,ENOEXEC:8,EBADF:9,ECHILD:10,EAGAIN:11,EWOULDBLOCK:11,ENOMEM:12,EACCES:13,EFAULT:14,ENOTBLK:15,EBUSY:16,EEXIST:17,EXDEV:18,ENODEV:19,ENOTDIR:20,EISDIR:21,EINVAL:22,ENFILE:23,EMFILE:24,ENOTTY:25,ETXTBSY:26,EFBIG:27,ENOSPC:28,ESPIPE:29,EROFS:30,EMLINK:31,EPIPE:32,EDOM:33,ERANGE:34,ENOMSG:42,EIDRM:43,ECHRNG:44,EL2NSYNC:45,EL3HLT:46,EL3RST:47,ELNRNG:48,EUNATCH:49,ENOCSI:50,EL2HLT:51,EDEADLK:35,ENOLCK:37,EBADE:52,EBADR:53,EXFULL:54,ENOANO:55,EBADRQC:56,EBADSLT:57,EDEADLOCK:35,EBFONT:59,ENOSTR:60,ENODATA:61,ETIME:62,ENOSR:63,ENONET:64,ENOPKG:65,EREMOTE:66,ENOLINK:67,EADV:68,ESRMNT:69,ECOMM:70,EPROTO:71,EMULTIHOP:72,EDOTDOT:73,EBADMSG:74,ENOTUNIQ:76,EBADFD:77,EREMCHG:78,ELIBACC:79,ELIBBAD:80,ELIBSCN:81,ELIBMAX:82,ELIBEXEC:83,ENOSYS:38,ENOTEMPTY:39,ENAMETOOLONG:36,ELOOP:40,EOPNOTSUPP:95,EPFNOSUPPORT:96,ECONNRESET:104,ENOBUFS:105,EAFNOSUPPORT:97,EPROTOTYPE:91,ENOTSOCK:88,ENOPROTOOPT:92,ESHUTDOWN:108,ECONNREFUSED:111,EADDRINUSE:98,ECONNABORTED:103,ENETUNREACH:101,ENETDOWN:100,ETIMEDOUT:110,EHOSTDOWN:112,EHOSTUNREACH:113,EINPROGRESS:115,EALREADY:114,EDESTADDRREQ:89,EMSGSIZE:90,EPROTONOSUPPORT:93,ESOCKTNOSUPPORT:94,EADDRNOTAVAIL:99,ENETRESET:102,EISCONN:106,ENOTCONN:107,ETOOMANYREFS:109,EUSERS:87,EDQUOT:122,ESTALE:116,ENOTSUP:95,ENOMEDIUM:123,EILSEQ:84,EOVERFLOW:75,ECANCELED:125,ENOTRECOVERABLE:131,EOWNERDEAD:130,ESTRPIPE:86};function _sysconf(name) {
      // long sysconf(int name);
      // http://pubs.opengroup.org/onlinepubs/009695399/functions/sysconf.html
      switch(name) {
        case 30: return PAGE_SIZE;
        case 132:
        case 133:
        case 12:
        case 137:
        case 138:
        case 15:
        case 235:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 149:
        case 13:
        case 10:
        case 236:
        case 153:
        case 9:
        case 21:
        case 22:
        case 159:
        case 154:
        case 14:
        case 77:
        case 78:
        case 139:
        case 80:
        case 81:
        case 79:
        case 82:
        case 68:
        case 67:
        case 164:
        case 11:
        case 29:
        case 47:
        case 48:
        case 95:
        case 52:
        case 51:
        case 46:
          return 200809;
        case 27:
        case 246:
        case 127:
        case 128:
        case 23:
        case 24:
        case 160:
        case 161:
        case 181:
        case 182:
        case 242:
        case 183:
        case 184:
        case 243:
        case 244:
        case 245:
        case 165:
        case 178:
        case 179:
        case 49:
        case 50:
        case 168:
        case 169:
        case 175:
        case 170:
        case 171:
        case 172:
        case 97:
        case 76:
        case 32:
        case 173:
        case 35:
          return -1;
        case 176:
        case 177:
        case 7:
        case 155:
        case 8:
        case 157:
        case 125:
        case 126:
        case 92:
        case 93:
        case 129:
        case 130:
        case 131:
        case 94:
        case 91:
          return 1;
        case 74:
        case 60:
        case 69:
        case 70:
        case 4:
          return 1024;
        case 31:
        case 42:
        case 72:
          return 32;
        case 87:
        case 26:
        case 33:
          return 2147483647;
        case 34:
        case 1:
          return 47839;
        case 38:
        case 36:
          return 99;
        case 43:
        case 37:
          return 2048;
        case 0: return 2097152;
        case 3: return 65536;
        case 28: return 32768;
        case 44: return 32767;
        case 75: return 16384;
        case 39: return 1000;
        case 89: return 700;
        case 71: return 256;
        case 40: return 255;
        case 2: return 100;
        case 180: return 64;
        case 25: return 20;
        case 5: return 16;
        case 6: return 6;
        case 73: return 4;
        case 84: return 1;
      }
      ___setErrNo(ERRNO_CODES.EINVAL);
      return -1;
    }

   
  Module["_memset"] = _memset;

  function ___errno_location() {
      return ___errno_state;
    }

  function _abort() {
      Module['abort']();
    }

  
  
  
  var ERRNO_MESSAGES={0:"Success",1:"Not super-user",2:"No such file or directory",3:"No such process",4:"Interrupted system call",5:"I/O error",6:"No such device or address",7:"Arg list too long",8:"Exec format error",9:"Bad file number",10:"No children",11:"No more processes",12:"Not enough core",13:"Permission denied",14:"Bad address",15:"Block device required",16:"Mount device busy",17:"File exists",18:"Cross-device link",19:"No such device",20:"Not a directory",21:"Is a directory",22:"Invalid argument",23:"Too many open files in system",24:"Too many open files",25:"Not a typewriter",26:"Text file busy",27:"File too large",28:"No space left on device",29:"Illegal seek",30:"Read only file system",31:"Too many links",32:"Broken pipe",33:"Math arg out of domain of func",34:"Math result not representable",35:"File locking deadlock error",36:"File or path name too long",37:"No record locks available",38:"Function not implemented",39:"Directory not empty",40:"Too many symbolic links",42:"No message of desired type",43:"Identifier removed",44:"Channel number out of range",45:"Level 2 not synchronized",46:"Level 3 halted",47:"Level 3 reset",48:"Link number out of range",49:"Protocol driver not attached",50:"No CSI structure available",51:"Level 2 halted",52:"Invalid exchange",53:"Invalid request descriptor",54:"Exchange full",55:"No anode",56:"Invalid request code",57:"Invalid slot",59:"Bad font file fmt",60:"Device not a stream",61:"No data (for no delay io)",62:"Timer expired",63:"Out of streams resources",64:"Machine is not on the network",65:"Package not installed",66:"The object is remote",67:"The link has been severed",68:"Advertise error",69:"Srmount error",70:"Communication error on send",71:"Protocol error",72:"Multihop attempted",73:"Cross mount point (not really error)",74:"Trying to read unreadable message",75:"Value too large for defined data type",76:"Given log. name not unique",77:"f.d. invalid for this operation",78:"Remote address changed",79:"Can   access a needed shared lib",80:"Accessing a corrupted shared lib",81:".lib section in a.out corrupted",82:"Attempting to link in too many libs",83:"Attempting to exec a shared library",84:"Illegal byte sequence",86:"Streams pipe error",87:"Too many users",88:"Socket operation on non-socket",89:"Destination address required",90:"Message too long",91:"Protocol wrong type for socket",92:"Protocol not available",93:"Unknown protocol",94:"Socket type not supported",95:"Not supported",96:"Protocol family not supported",97:"Address family not supported by protocol family",98:"Address already in use",99:"Address not available",100:"Network interface is not configured",101:"Network is unreachable",102:"Connection reset by network",103:"Connection aborted",104:"Connection reset by peer",105:"No buffer space available",106:"Socket is already connected",107:"Socket is not connected",108:"Can't send after socket shutdown",109:"Too many references",110:"Connection timed out",111:"Connection refused",112:"Host is down",113:"Host is unreachable",114:"Socket already connected",115:"Connection already in progress",116:"Stale file handle",122:"Quota exceeded",123:"No medium (in tape drive)",125:"Operation canceled",130:"Previous owner died",131:"State not recoverable"};
  
  var TTY={ttys:[],init:function () {
        // https://github.com/kripken/emscripten/pull/1555
        // if (ENVIRONMENT_IS_NODE) {
        //   // currently, FS.init does not distinguish if process.stdin is a file or TTY
        //   // device, it always assumes it's a TTY device. because of this, we're forcing
        //   // process.stdin to UTF8 encoding to at least make stdin reading compatible
        //   // with text files until FS.init can be refactored.
        //   process['stdin']['setEncoding']('utf8');
        // }
      },shutdown:function () {
        // https://github.com/kripken/emscripten/pull/1555
        // if (ENVIRONMENT_IS_NODE) {
        //   // inolen: any idea as to why node -e 'process.stdin.read()' wouldn't exit immediately (with process.stdin being a tty)?
        //   // isaacs: because now it's reading from the stream, you've expressed interest in it, so that read() kicks off a _read() which creates a ReadReq operation
        //   // inolen: I thought read() in that case was a synchronous operation that just grabbed some amount of buffered data if it exists?
        //   // isaacs: it is. but it also triggers a _read() call, which calls readStart() on the handle
        //   // isaacs: do process.stdin.pause() and i'd think it'd probably close the pending call
        //   process['stdin']['pause']();
        // }
      },register:function (dev, ops) {
        TTY.ttys[dev] = { input: [], output: [], ops: ops };
        FS.registerDevice(dev, TTY.stream_ops);
      },stream_ops:{open:function (stream) {
          var tty = TTY.ttys[stream.node.rdev];
          if (!tty) {
            throw new FS.ErrnoError(ERRNO_CODES.ENODEV);
          }
          stream.tty = tty;
          stream.seekable = false;
        },close:function (stream) {
          // flush any pending line data
          if (stream.tty.output.length) {
            stream.tty.ops.put_char(stream.tty, 10);
          }
        },read:function (stream, buffer, offset, length, pos /* ignored */) {
          if (!stream.tty || !stream.tty.ops.get_char) {
            throw new FS.ErrnoError(ERRNO_CODES.ENXIO);
          }
          var bytesRead = 0;
          for (var i = 0; i < length; i++) {
            var result;
            try {
              result = stream.tty.ops.get_char(stream.tty);
            } catch (e) {
              throw new FS.ErrnoError(ERRNO_CODES.EIO);
            }
            if (result === undefined && bytesRead === 0) {
              throw new FS.ErrnoError(ERRNO_CODES.EAGAIN);
            }
            if (result === null || result === undefined) break;
            bytesRead++;
            buffer[offset+i] = result;
          }
          if (bytesRead) {
            stream.node.timestamp = Date.now();
          }
          return bytesRead;
        },write:function (stream, buffer, offset, length, pos) {
          if (!stream.tty || !stream.tty.ops.put_char) {
            throw new FS.ErrnoError(ERRNO_CODES.ENXIO);
          }
          for (var i = 0; i < length; i++) {
            try {
              stream.tty.ops.put_char(stream.tty, buffer[offset+i]);
            } catch (e) {
              throw new FS.ErrnoError(ERRNO_CODES.EIO);
            }
          }
          if (length) {
            stream.node.timestamp = Date.now();
          }
          return i;
        }},default_tty_ops:{get_char:function (tty) {
          if (!tty.input.length) {
            var result = null;
            if (ENVIRONMENT_IS_NODE) {
              result = process['stdin']['read']();
              if (!result) {
                if (process['stdin']['_readableState'] && process['stdin']['_readableState']['ended']) {
                  return null;  // EOF
                }
                return undefined;  // no data available
              }
            } else if (typeof window != 'undefined' &&
              typeof window.prompt == 'function') {
              // Browser.
              result = window.prompt('Input: ');  // returns null on cancel
              if (result !== null) {
                result += '\n';
              }
            } else if (typeof readline == 'function') {
              // Command line.
              result = readline();
              if (result !== null) {
                result += '\n';
              }
            }
            if (!result) {
              return null;
            }
            tty.input = intArrayFromString(result, true);
          }
          return tty.input.shift();
        },put_char:function (tty, val) {
          if (val === null || val === 10) {
            Module['print'](tty.output.join(''));
            tty.output = [];
          } else {
            tty.output.push(TTY.utf8.processCChar(val));
          }
        }},default_tty1_ops:{put_char:function (tty, val) {
          if (val === null || val === 10) {
            Module['printErr'](tty.output.join(''));
            tty.output = [];
          } else {
            tty.output.push(TTY.utf8.processCChar(val));
          }
        }}};
  
  var MEMFS={ops_table:null,CONTENT_OWNING:1,CONTENT_FLEXIBLE:2,CONTENT_FIXED:3,mount:function (mount) {
        return MEMFS.createNode(null, '/', 16384 | 511 /* 0777 */, 0);
      },createNode:function (parent, name, mode, dev) {
        if (FS.isBlkdev(mode) || FS.isFIFO(mode)) {
          // no supported
          throw new FS.ErrnoError(ERRNO_CODES.EPERM);
        }
        if (!MEMFS.ops_table) {
          MEMFS.ops_table = {
            dir: {
              node: {
                getattr: MEMFS.node_ops.getattr,
                setattr: MEMFS.node_ops.setattr,
                lookup: MEMFS.node_ops.lookup,
                mknod: MEMFS.node_ops.mknod,
                rename: MEMFS.node_ops.rename,
                unlink: MEMFS.node_ops.unlink,
                rmdir: MEMFS.node_ops.rmdir,
                readdir: MEMFS.node_ops.readdir,
                symlink: MEMFS.node_ops.symlink
              },
              stream: {
                llseek: MEMFS.stream_ops.llseek
              }
            },
            file: {
              node: {
                getattr: MEMFS.node_ops.getattr,
                setattr: MEMFS.node_ops.setattr
              },
              stream: {
                llseek: MEMFS.stream_ops.llseek,
                read: MEMFS.stream_ops.read,
                write: MEMFS.stream_ops.write,
                allocate: MEMFS.stream_ops.allocate,
                mmap: MEMFS.stream_ops.mmap
              }
            },
            link: {
              node: {
                getattr: MEMFS.node_ops.getattr,
                setattr: MEMFS.node_ops.setattr,
                readlink: MEMFS.node_ops.readlink
              },
              stream: {}
            },
            chrdev: {
              node: {
                getattr: MEMFS.node_ops.getattr,
                setattr: MEMFS.node_ops.setattr
              },
              stream: FS.chrdev_stream_ops
            },
          };
        }
        var node = FS.createNode(parent, name, mode, dev);
        if (FS.isDir(node.mode)) {
          node.node_ops = MEMFS.ops_table.dir.node;
          node.stream_ops = MEMFS.ops_table.dir.stream;
          node.contents = {};
        } else if (FS.isFile(node.mode)) {
          node.node_ops = MEMFS.ops_table.file.node;
          node.stream_ops = MEMFS.ops_table.file.stream;
          node.contents = [];
          node.contentMode = MEMFS.CONTENT_FLEXIBLE;
        } else if (FS.isLink(node.mode)) {
          node.node_ops = MEMFS.ops_table.link.node;
          node.stream_ops = MEMFS.ops_table.link.stream;
        } else if (FS.isChrdev(node.mode)) {
          node.node_ops = MEMFS.ops_table.chrdev.node;
          node.stream_ops = MEMFS.ops_table.chrdev.stream;
        }
        node.timestamp = Date.now();
        // add the new node to the parent
        if (parent) {
          parent.contents[name] = node;
        }
        return node;
      },ensureFlexible:function (node) {
        if (node.contentMode !== MEMFS.CONTENT_FLEXIBLE) {
          var contents = node.contents;
          node.contents = Array.prototype.slice.call(contents);
          node.contentMode = MEMFS.CONTENT_FLEXIBLE;
        }
      },node_ops:{getattr:function (node) {
          var attr = {};
          // device numbers reuse inode numbers.
          attr.dev = FS.isChrdev(node.mode) ? node.id : 1;
          attr.ino = node.id;
          attr.mode = node.mode;
          attr.nlink = 1;
          attr.uid = 0;
          attr.gid = 0;
          attr.rdev = node.rdev;
          if (FS.isDir(node.mode)) {
            attr.size = 4096;
          } else if (FS.isFile(node.mode)) {
            attr.size = node.contents.length;
          } else if (FS.isLink(node.mode)) {
            attr.size = node.link.length;
          } else {
            attr.size = 0;
          }
          attr.atime = new Date(node.timestamp);
          attr.mtime = new Date(node.timestamp);
          attr.ctime = new Date(node.timestamp);
          // NOTE: In our implementation, st_blocks = Math.ceil(st_size/st_blksize),
          //       but this is not required by the standard.
          attr.blksize = 4096;
          attr.blocks = Math.ceil(attr.size / attr.blksize);
          return attr;
        },setattr:function (node, attr) {
          if (attr.mode !== undefined) {
            node.mode = attr.mode;
          }
          if (attr.timestamp !== undefined) {
            node.timestamp = attr.timestamp;
          }
          if (attr.size !== undefined) {
            MEMFS.ensureFlexible(node);
            var contents = node.contents;
            if (attr.size < contents.length) contents.length = attr.size;
            else while (attr.size > contents.length) contents.push(0);
          }
        },lookup:function (parent, name) {
          throw FS.genericErrors[ERRNO_CODES.ENOENT];
        },mknod:function (parent, name, mode, dev) {
          return MEMFS.createNode(parent, name, mode, dev);
        },rename:function (old_node, new_dir, new_name) {
          // if we're overwriting a directory at new_name, make sure it's empty.
          if (FS.isDir(old_node.mode)) {
            var new_node;
            try {
              new_node = FS.lookupNode(new_dir, new_name);
            } catch (e) {
            }
            if (new_node) {
              for (var i in new_node.contents) {
                throw new FS.ErrnoError(ERRNO_CODES.ENOTEMPTY);
              }
            }
          }
          // do the internal rewiring
          delete old_node.parent.contents[old_node.name];
          old_node.name = new_name;
          new_dir.contents[new_name] = old_node;
          old_node.parent = new_dir;
        },unlink:function (parent, name) {
          delete parent.contents[name];
        },rmdir:function (parent, name) {
          var node = FS.lookupNode(parent, name);
          for (var i in node.contents) {
            throw new FS.ErrnoError(ERRNO_CODES.ENOTEMPTY);
          }
          delete parent.contents[name];
        },readdir:function (node) {
          var entries = ['.', '..']
          for (var key in node.contents) {
            if (!node.contents.hasOwnProperty(key)) {
              continue;
            }
            entries.push(key);
          }
          return entries;
        },symlink:function (parent, newname, oldpath) {
          var node = MEMFS.createNode(parent, newname, 511 /* 0777 */ | 40960, 0);
          node.link = oldpath;
          return node;
        },readlink:function (node) {
          if (!FS.isLink(node.mode)) {
            throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
          }
          return node.link;
        }},stream_ops:{read:function (stream, buffer, offset, length, position) {
          var contents = stream.node.contents;
          if (position >= contents.length)
            return 0;
          var size = Math.min(contents.length - position, length);
          assert(size >= 0);
          if (size > 8 && contents.subarray) { // non-trivial, and typed array
            buffer.set(contents.subarray(position, position + size), offset);
          } else
          {
            for (var i = 0; i < size; i++) {
              buffer[offset + i] = contents[position + i];
            }
          }
          return size;
        },write:function (stream, buffer, offset, length, position, canOwn) {
          var node = stream.node;
          node.timestamp = Date.now();
          var contents = node.contents;
          if (length && contents.length === 0 && position === 0 && buffer.subarray) {
            // just replace it with the new data
            assert(buffer.length);
            if (canOwn && offset === 0) {
              node.contents = buffer; // this could be a subarray of Emscripten HEAP, or allocated from some other source.
              node.contentMode = (buffer.buffer === HEAP8.buffer) ? MEMFS.CONTENT_OWNING : MEMFS.CONTENT_FIXED;
            } else {
              node.contents = new Uint8Array(buffer.subarray(offset, offset+length));
              node.contentMode = MEMFS.CONTENT_FIXED;
            }
            return length;
          }
          MEMFS.ensureFlexible(node);
          var contents = node.contents;
          while (contents.length < position) contents.push(0);
          for (var i = 0; i < length; i++) {
            contents[position + i] = buffer[offset + i];
          }
          return length;
        },llseek:function (stream, offset, whence) {
          var position = offset;
          if (whence === 1) {  // SEEK_CUR.
            position += stream.position;
          } else if (whence === 2) {  // SEEK_END.
            if (FS.isFile(stream.node.mode)) {
              position += stream.node.contents.length;
            }
          }
          if (position < 0) {
            throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
          }
          stream.ungotten = [];
          stream.position = position;
          return position;
        },allocate:function (stream, offset, length) {
          MEMFS.ensureFlexible(stream.node);
          var contents = stream.node.contents;
          var limit = offset + length;
          while (limit > contents.length) contents.push(0);
        },mmap:function (stream, buffer, offset, length, position, prot, flags) {
          if (!FS.isFile(stream.node.mode)) {
            throw new FS.ErrnoError(ERRNO_CODES.ENODEV);
          }
          var ptr;
          var allocated;
          var contents = stream.node.contents;
          // Only make a new copy when MAP_PRIVATE is specified.
          if ( !(flags & 2) &&
                (contents.buffer === buffer || contents.buffer === buffer.buffer) ) {
            // We can't emulate MAP_SHARED when the file is not backed by the buffer
            // we're mapping to (e.g. the HEAP buffer).
            allocated = false;
            ptr = contents.byteOffset;
          } else {
            // Try to avoid unnecessary slices.
            if (position > 0 || position + length < contents.length) {
              if (contents.subarray) {
                contents = contents.subarray(position, position + length);
              } else {
                contents = Array.prototype.slice.call(contents, position, position + length);
              }
            }
            allocated = true;
            ptr = _malloc(length);
            if (!ptr) {
              throw new FS.ErrnoError(ERRNO_CODES.ENOMEM);
            }
            buffer.set(contents, ptr);
          }
          return { ptr: ptr, allocated: allocated };
        }}};
  
  var IDBFS={dbs:{},indexedDB:function () {
        return window.indexedDB || window.mozIndexedDB || window.webkitIndexedDB || window.msIndexedDB;
      },DB_VERSION:21,DB_STORE_NAME:"FILE_DATA",mount:function (mount) {
        // reuse all of the core MEMFS functionality
        return MEMFS.mount.apply(null, arguments);
      },syncfs:function (mount, populate, callback) {
        IDBFS.getLocalSet(mount, function(err, local) {
          if (err) return callback(err);
  
          IDBFS.getRemoteSet(mount, function(err, remote) {
            if (err) return callback(err);
  
            var src = populate ? remote : local;
            var dst = populate ? local : remote;
  
            IDBFS.reconcile(src, dst, callback);
          });
        });
      },getDB:function (name, callback) {
        // check the cache first
        var db = IDBFS.dbs[name];
        if (db) {
          return callback(null, db);
        }
  
        var req;
        try {
          req = IDBFS.indexedDB().open(name, IDBFS.DB_VERSION);
        } catch (e) {
          return callback(e);
        }
        req.onupgradeneeded = function(e) {
          var db = e.target.result;
          var transaction = e.target.transaction;
  
          var fileStore;
  
          if (db.objectStoreNames.contains(IDBFS.DB_STORE_NAME)) {
            fileStore = transaction.objectStore(IDBFS.DB_STORE_NAME);
          } else {
            fileStore = db.createObjectStore(IDBFS.DB_STORE_NAME);
          }
  
          fileStore.createIndex('timestamp', 'timestamp', { unique: false });
        };
        req.onsuccess = function() {
          db = req.result;
  
          // add to the cache
          IDBFS.dbs[name] = db;
          callback(null, db);
        };
        req.onerror = function() {
          callback(this.error);
        };
      },getLocalSet:function (mount, callback) {
        var entries = {};
  
        function isRealDir(p) {
          return p !== '.' && p !== '..';
        };
        function toAbsolute(root) {
          return function(p) {
            return PATH.join2(root, p);
          }
        };
  
        var check = FS.readdir(mount.mountpoint).filter(isRealDir).map(toAbsolute(mount.mountpoint));
  
        while (check.length) {
          var path = check.pop();
          var stat;
  
          try {
            stat = FS.stat(path);
          } catch (e) {
            return callback(e);
          }
  
          if (FS.isDir(stat.mode)) {
            check.push.apply(check, FS.readdir(path).filter(isRealDir).map(toAbsolute(path)));
          }
  
          entries[path] = { timestamp: stat.mtime };
        }
  
        return callback(null, { type: 'local', entries: entries });
      },getRemoteSet:function (mount, callback) {
        var entries = {};
  
        IDBFS.getDB(mount.mountpoint, function(err, db) {
          if (err) return callback(err);
  
          var transaction = db.transaction([IDBFS.DB_STORE_NAME], 'readonly');
          transaction.onerror = function() { callback(this.error); };
  
          var store = transaction.objectStore(IDBFS.DB_STORE_NAME);
          var index = store.index('timestamp');
  
          index.openKeyCursor().onsuccess = function(event) {
            var cursor = event.target.result;
  
            if (!cursor) {
              return callback(null, { type: 'remote', db: db, entries: entries });
            }
  
            entries[cursor.primaryKey] = { timestamp: cursor.key };
  
            cursor.continue();
          };
        });
      },loadLocalEntry:function (path, callback) {
        var stat, node;
  
        try {
          var lookup = FS.lookupPath(path);
          node = lookup.node;
          stat = FS.stat(path);
        } catch (e) {
          return callback(e);
        }
  
        if (FS.isDir(stat.mode)) {
          return callback(null, { timestamp: stat.mtime, mode: stat.mode });
        } else if (FS.isFile(stat.mode)) {
          return callback(null, { timestamp: stat.mtime, mode: stat.mode, contents: node.contents });
        } else {
          return callback(new Error('node type not supported'));
        }
      },storeLocalEntry:function (path, entry, callback) {
        try {
          if (FS.isDir(entry.mode)) {
            FS.mkdir(path, entry.mode);
          } else if (FS.isFile(entry.mode)) {
            FS.writeFile(path, entry.contents, { encoding: 'binary', canOwn: true });
          } else {
            return callback(new Error('node type not supported'));
          }
  
          FS.utime(path, entry.timestamp, entry.timestamp);
        } catch (e) {
          return callback(e);
        }
  
        callback(null);
      },removeLocalEntry:function (path, callback) {
        try {
          var lookup = FS.lookupPath(path);
          var stat = FS.stat(path);
  
          if (FS.isDir(stat.mode)) {
            FS.rmdir(path);
          } else if (FS.isFile(stat.mode)) {
            FS.unlink(path);
          }
        } catch (e) {
          return callback(e);
        }
  
        callback(null);
      },loadRemoteEntry:function (store, path, callback) {
        var req = store.get(path);
        req.onsuccess = function(event) { callback(null, event.target.result); };
        req.onerror = function() { callback(this.error); };
      },storeRemoteEntry:function (store, path, entry, callback) {
        var req = store.put(entry, path);
        req.onsuccess = function() { callback(null); };
        req.onerror = function() { callback(this.error); };
      },removeRemoteEntry:function (store, path, callback) {
        var req = store.delete(path);
        req.onsuccess = function() { callback(null); };
        req.onerror = function() { callback(this.error); };
      },reconcile:function (src, dst, callback) {
        var total = 0;
  
        var create = [];
        Object.keys(src.entries).forEach(function (key) {
          var e = src.entries[key];
          var e2 = dst.entries[key];
          if (!e2 || e.timestamp > e2.timestamp) {
            create.push(key);
            total++;
          }
        });
  
        var remove = [];
        Object.keys(dst.entries).forEach(function (key) {
          var e = dst.entries[key];
          var e2 = src.entries[key];
          if (!e2) {
            remove.push(key);
            total++;
          }
        });
  
        if (!total) {
          return callback(null);
        }
  
        var errored = false;
        var completed = 0;
        var db = src.type === 'remote' ? src.db : dst.db;
        var transaction = db.transaction([IDBFS.DB_STORE_NAME], 'readwrite');
        var store = transaction.objectStore(IDBFS.DB_STORE_NAME);
  
        function done(err) {
          if (err) {
            if (!done.errored) {
              done.errored = true;
              return callback(err);
            }
            return;
          }
          if (++completed >= total) {
            return callback(null);
          }
        };
  
        transaction.onerror = function() { done(this.error); };
  
        // sort paths in ascending order so directory entries are created
        // before the files inside them
        create.sort().forEach(function (path) {
          if (dst.type === 'local') {
            IDBFS.loadRemoteEntry(store, path, function (err, entry) {
              if (err) return done(err);
              IDBFS.storeLocalEntry(path, entry, done);
            });
          } else {
            IDBFS.loadLocalEntry(path, function (err, entry) {
              if (err) return done(err);
              IDBFS.storeRemoteEntry(store, path, entry, done);
            });
          }
        });
  
        // sort paths in descending order so files are deleted before their
        // parent directories
        remove.sort().reverse().forEach(function(path) {
          if (dst.type === 'local') {
            IDBFS.removeLocalEntry(path, done);
          } else {
            IDBFS.removeRemoteEntry(store, path, done);
          }
        });
      }};
  
  var NODEFS={isWindows:false,staticInit:function () {
        NODEFS.isWindows = !!process.platform.match(/^win/);
      },mount:function (mount) {
        assert(ENVIRONMENT_IS_NODE);
        return NODEFS.createNode(null, '/', NODEFS.getMode(mount.opts.root), 0);
      },createNode:function (parent, name, mode, dev) {
        if (!FS.isDir(mode) && !FS.isFile(mode) && !FS.isLink(mode)) {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
        var node = FS.createNode(parent, name, mode);
        node.node_ops = NODEFS.node_ops;
        node.stream_ops = NODEFS.stream_ops;
        return node;
      },getMode:function (path) {
        var stat;
        try {
          stat = fs.lstatSync(path);
          if (NODEFS.isWindows) {
            // On Windows, directories return permission bits 'rw-rw-rw-', even though they have 'rwxrwxrwx', so 
            // propagate write bits to execute bits.
            stat.mode = stat.mode | ((stat.mode & 146) >> 1);
          }
        } catch (e) {
          if (!e.code) throw e;
          throw new FS.ErrnoError(ERRNO_CODES[e.code]);
        }
        return stat.mode;
      },realPath:function (node) {
        var parts = [];
        while (node.parent !== node) {
          parts.push(node.name);
          node = node.parent;
        }
        parts.push(node.mount.opts.root);
        parts.reverse();
        return PATH.join.apply(null, parts);
      },flagsToPermissionStringMap:{0:"r",1:"r+",2:"r+",64:"r",65:"r+",66:"r+",129:"rx+",193:"rx+",514:"w+",577:"w",578:"w+",705:"wx",706:"wx+",1024:"a",1025:"a",1026:"a+",1089:"a",1090:"a+",1153:"ax",1154:"ax+",1217:"ax",1218:"ax+",4096:"rs",4098:"rs+"},flagsToPermissionString:function (flags) {
        if (flags in NODEFS.flagsToPermissionStringMap) {
          return NODEFS.flagsToPermissionStringMap[flags];
        } else {
          return flags;
        }
      },node_ops:{getattr:function (node) {
          var path = NODEFS.realPath(node);
          var stat;
          try {
            stat = fs.lstatSync(path);
          } catch (e) {
            if (!e.code) throw e;
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
          // node.js v0.10.20 doesn't report blksize and blocks on Windows. Fake them with default blksize of 4096.
          // See http://support.microsoft.com/kb/140365
          if (NODEFS.isWindows && !stat.blksize) {
            stat.blksize = 4096;
          }
          if (NODEFS.isWindows && !stat.blocks) {
            stat.blocks = (stat.size+stat.blksize-1)/stat.blksize|0;
          }
          return {
            dev: stat.dev,
            ino: stat.ino,
            mode: stat.mode,
            nlink: stat.nlink,
            uid: stat.uid,
            gid: stat.gid,
            rdev: stat.rdev,
            size: stat.size,
            atime: stat.atime,
            mtime: stat.mtime,
            ctime: stat.ctime,
            blksize: stat.blksize,
            blocks: stat.blocks
          };
        },setattr:function (node, attr) {
          var path = NODEFS.realPath(node);
          try {
            if (attr.mode !== undefined) {
              fs.chmodSync(path, attr.mode);
              // update the common node structure mode as well
              node.mode = attr.mode;
            }
            if (attr.timestamp !== undefined) {
              var date = new Date(attr.timestamp);
              fs.utimesSync(path, date, date);
            }
            if (attr.size !== undefined) {
              fs.truncateSync(path, attr.size);
            }
          } catch (e) {
            if (!e.code) throw e;
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
        },lookup:function (parent, name) {
          var path = PATH.join2(NODEFS.realPath(parent), name);
          var mode = NODEFS.getMode(path);
          return NODEFS.createNode(parent, name, mode);
        },mknod:function (parent, name, mode, dev) {
          var node = NODEFS.createNode(parent, name, mode, dev);
          // create the backing node for this in the fs root as well
          var path = NODEFS.realPath(node);
          try {
            if (FS.isDir(node.mode)) {
              fs.mkdirSync(path, node.mode);
            } else {
              fs.writeFileSync(path, '', { mode: node.mode });
            }
          } catch (e) {
            if (!e.code) throw e;
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
          return node;
        },rename:function (oldNode, newDir, newName) {
          var oldPath = NODEFS.realPath(oldNode);
          var newPath = PATH.join2(NODEFS.realPath(newDir), newName);
          try {
            fs.renameSync(oldPath, newPath);
          } catch (e) {
            if (!e.code) throw e;
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
        },unlink:function (parent, name) {
          var path = PATH.join2(NODEFS.realPath(parent), name);
          try {
            fs.unlinkSync(path);
          } catch (e) {
            if (!e.code) throw e;
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
        },rmdir:function (parent, name) {
          var path = PATH.join2(NODEFS.realPath(parent), name);
          try {
            fs.rmdirSync(path);
          } catch (e) {
            if (!e.code) throw e;
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
        },readdir:function (node) {
          var path = NODEFS.realPath(node);
          try {
            return fs.readdirSync(path);
          } catch (e) {
            if (!e.code) throw e;
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
        },symlink:function (parent, newName, oldPath) {
          var newPath = PATH.join2(NODEFS.realPath(parent), newName);
          try {
            fs.symlinkSync(oldPath, newPath);
          } catch (e) {
            if (!e.code) throw e;
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
        },readlink:function (node) {
          var path = NODEFS.realPath(node);
          try {
            return fs.readlinkSync(path);
          } catch (e) {
            if (!e.code) throw e;
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
        }},stream_ops:{open:function (stream) {
          var path = NODEFS.realPath(stream.node);
          try {
            if (FS.isFile(stream.node.mode)) {
              stream.nfd = fs.openSync(path, NODEFS.flagsToPermissionString(stream.flags));
            }
          } catch (e) {
            if (!e.code) throw e;
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
        },close:function (stream) {
          try {
            if (FS.isFile(stream.node.mode) && stream.nfd) {
              fs.closeSync(stream.nfd);
            }
          } catch (e) {
            if (!e.code) throw e;
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
        },read:function (stream, buffer, offset, length, position) {
          // FIXME this is terrible.
          var nbuffer = new Buffer(length);
          var res;
          try {
            res = fs.readSync(stream.nfd, nbuffer, 0, length, position);
          } catch (e) {
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
          if (res > 0) {
            for (var i = 0; i < res; i++) {
              buffer[offset + i] = nbuffer[i];
            }
          }
          return res;
        },write:function (stream, buffer, offset, length, position) {
          // FIXME this is terrible.
          var nbuffer = new Buffer(buffer.subarray(offset, offset + length));
          var res;
          try {
            res = fs.writeSync(stream.nfd, nbuffer, 0, length, position);
          } catch (e) {
            throw new FS.ErrnoError(ERRNO_CODES[e.code]);
          }
          return res;
        },llseek:function (stream, offset, whence) {
          var position = offset;
          if (whence === 1) {  // SEEK_CUR.
            position += stream.position;
          } else if (whence === 2) {  // SEEK_END.
            if (FS.isFile(stream.node.mode)) {
              try {
                var stat = fs.fstatSync(stream.nfd);
                position += stat.size;
              } catch (e) {
                throw new FS.ErrnoError(ERRNO_CODES[e.code]);
              }
            }
          }
  
          if (position < 0) {
            throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
          }
  
          stream.position = position;
          return position;
        }}};
  
  var _stdin=allocate(1, "i32*", ALLOC_STATIC);
  
  var _stdout=allocate(1, "i32*", ALLOC_STATIC);
  
  var _stderr=allocate(1, "i32*", ALLOC_STATIC);
  
  function _fflush(stream) {
      // int fflush(FILE *stream);
      // http://pubs.opengroup.org/onlinepubs/000095399/functions/fflush.html
      // we don't currently perform any user-space buffering of data
    }var FS={root:null,mounts:[],devices:[null],streams:[],nextInode:1,nameTable:null,currentPath:"/",initialized:false,ignorePermissions:true,ErrnoError:null,genericErrors:{},handleFSError:function (e) {
        if (!(e instanceof FS.ErrnoError)) throw e + ' : ' + stackTrace();
        return ___setErrNo(e.errno);
      },lookupPath:function (path, opts) {
        path = PATH.resolve(FS.cwd(), path);
        opts = opts || {};
  
        var defaults = {
          follow_mount: true,
          recurse_count: 0
        };
        for (var key in defaults) {
          if (opts[key] === undefined) {
            opts[key] = defaults[key];
          }
        }
  
        if (opts.recurse_count > 8) {  // max recursive lookup of 8
          throw new FS.ErrnoError(ERRNO_CODES.ELOOP);
        }
  
        // split the path
        var parts = PATH.normalizeArray(path.split('/').filter(function(p) {
          return !!p;
        }), false);
  
        // start at the root
        var current = FS.root;
        var current_path = '/';
  
        for (var i = 0; i < parts.length; i++) {
          var islast = (i === parts.length-1);
          if (islast && opts.parent) {
            // stop resolving
            break;
          }
  
          current = FS.lookupNode(current, parts[i]);
          current_path = PATH.join2(current_path, parts[i]);
  
          // jump to the mount's root node if this is a mountpoint
          if (FS.isMountpoint(current)) {
            if (!islast || (islast && opts.follow_mount)) {
              current = current.mounted.root;
            }
          }
  
          // by default, lookupPath will not follow a symlink if it is the final path component.
          // setting opts.follow = true will override this behavior.
          if (!islast || opts.follow) {
            var count = 0;
            while (FS.isLink(current.mode)) {
              var link = FS.readlink(current_path);
              current_path = PATH.resolve(PATH.dirname(current_path), link);
              
              var lookup = FS.lookupPath(current_path, { recurse_count: opts.recurse_count });
              current = lookup.node;
  
              if (count++ > 40) {  // limit max consecutive symlinks to 40 (SYMLOOP_MAX).
                throw new FS.ErrnoError(ERRNO_CODES.ELOOP);
              }
            }
          }
        }
  
        return { path: current_path, node: current };
      },getPath:function (node) {
        var path;
        while (true) {
          if (FS.isRoot(node)) {
            var mount = node.mount.mountpoint;
            if (!path) return mount;
            return mount[mount.length-1] !== '/' ? mount + '/' + path : mount + path;
          }
          path = path ? node.name + '/' + path : node.name;
          node = node.parent;
        }
      },hashName:function (parentid, name) {
        var hash = 0;
  
  
        for (var i = 0; i < name.length; i++) {
          hash = ((hash << 5) - hash + name.charCodeAt(i)) | 0;
        }
        return ((parentid + hash) >>> 0) % FS.nameTable.length;
      },hashAddNode:function (node) {
        var hash = FS.hashName(node.parent.id, node.name);
        node.name_next = FS.nameTable[hash];
        FS.nameTable[hash] = node;
      },hashRemoveNode:function (node) {
        var hash = FS.hashName(node.parent.id, node.name);
        if (FS.nameTable[hash] === node) {
          FS.nameTable[hash] = node.name_next;
        } else {
          var current = FS.nameTable[hash];
          while (current) {
            if (current.name_next === node) {
              current.name_next = node.name_next;
              break;
            }
            current = current.name_next;
          }
        }
      },lookupNode:function (parent, name) {
        var err = FS.mayLookup(parent);
        if (err) {
          throw new FS.ErrnoError(err);
        }
        var hash = FS.hashName(parent.id, name);
        for (var node = FS.nameTable[hash]; node; node = node.name_next) {
          var nodeName = node.name;
          if (node.parent.id === parent.id && nodeName === name) {
            return node;
          }
        }
        // if we failed to find it in the cache, call into the VFS
        return FS.lookup(parent, name);
      },createNode:function (parent, name, mode, rdev) {
        if (!FS.FSNode) {
          FS.FSNode = function(parent, name, mode, rdev) {
            if (!parent) {
              parent = this;  // root node sets parent to itself
            }
            this.parent = parent;
            this.mount = parent.mount;
            this.mounted = null;
            this.id = FS.nextInode++;
            this.name = name;
            this.mode = mode;
            this.node_ops = {};
            this.stream_ops = {};
            this.rdev = rdev;
          };
  
          FS.FSNode.prototype = {};
  
          // compatibility
          var readMode = 292 | 73;
          var writeMode = 146;
  
          // NOTE we must use Object.defineProperties instead of individual calls to
          // Object.defineProperty in order to make closure compiler happy
          Object.defineProperties(FS.FSNode.prototype, {
            read: {
              get: function() { return (this.mode & readMode) === readMode; },
              set: function(val) { val ? this.mode |= readMode : this.mode &= ~readMode; }
            },
            write: {
              get: function() { return (this.mode & writeMode) === writeMode; },
              set: function(val) { val ? this.mode |= writeMode : this.mode &= ~writeMode; }
            },
            isFolder: {
              get: function() { return FS.isDir(this.mode); },
            },
            isDevice: {
              get: function() { return FS.isChrdev(this.mode); },
            },
          });
        }
  
        var node = new FS.FSNode(parent, name, mode, rdev);
  
        FS.hashAddNode(node);
  
        return node;
      },destroyNode:function (node) {
        FS.hashRemoveNode(node);
      },isRoot:function (node) {
        return node === node.parent;
      },isMountpoint:function (node) {
        return !!node.mounted;
      },isFile:function (mode) {
        return (mode & 61440) === 32768;
      },isDir:function (mode) {
        return (mode & 61440) === 16384;
      },isLink:function (mode) {
        return (mode & 61440) === 40960;
      },isChrdev:function (mode) {
        return (mode & 61440) === 8192;
      },isBlkdev:function (mode) {
        return (mode & 61440) === 24576;
      },isFIFO:function (mode) {
        return (mode & 61440) === 4096;
      },isSocket:function (mode) {
        return (mode & 49152) === 49152;
      },flagModes:{"r":0,"rs":1052672,"r+":2,"w":577,"wx":705,"xw":705,"w+":578,"wx+":706,"xw+":706,"a":1089,"ax":1217,"xa":1217,"a+":1090,"ax+":1218,"xa+":1218},modeStringToFlags:function (str) {
        var flags = FS.flagModes[str];
        if (typeof flags === 'undefined') {
          throw new Error('Unknown file open mode: ' + str);
        }
        return flags;
      },flagsToPermissionString:function (flag) {
        var accmode = flag & 2097155;
        var perms = ['r', 'w', 'rw'][accmode];
        if ((flag & 512)) {
          perms += 'w';
        }
        return perms;
      },nodePermissions:function (node, perms) {
        if (FS.ignorePermissions) {
          return 0;
        }
        // return 0 if any user, group or owner bits are set.
        if (perms.indexOf('r') !== -1 && !(node.mode & 292)) {
          return ERRNO_CODES.EACCES;
        } else if (perms.indexOf('w') !== -1 && !(node.mode & 146)) {
          return ERRNO_CODES.EACCES;
        } else if (perms.indexOf('x') !== -1 && !(node.mode & 73)) {
          return ERRNO_CODES.EACCES;
        }
        return 0;
      },mayLookup:function (dir) {
        return FS.nodePermissions(dir, 'x');
      },mayCreate:function (dir, name) {
        try {
          var node = FS.lookupNode(dir, name);
          return ERRNO_CODES.EEXIST;
        } catch (e) {
        }
        return FS.nodePermissions(dir, 'wx');
      },mayDelete:function (dir, name, isdir) {
        var node;
        try {
          node = FS.lookupNode(dir, name);
        } catch (e) {
          return e.errno;
        }
        var err = FS.nodePermissions(dir, 'wx');
        if (err) {
          return err;
        }
        if (isdir) {
          if (!FS.isDir(node.mode)) {
            return ERRNO_CODES.ENOTDIR;
          }
          if (FS.isRoot(node) || FS.getPath(node) === FS.cwd()) {
            return ERRNO_CODES.EBUSY;
          }
        } else {
          if (FS.isDir(node.mode)) {
            return ERRNO_CODES.EISDIR;
          }
        }
        return 0;
      },mayOpen:function (node, flags) {
        if (!node) {
          return ERRNO_CODES.ENOENT;
        }
        if (FS.isLink(node.mode)) {
          return ERRNO_CODES.ELOOP;
        } else if (FS.isDir(node.mode)) {
          if ((flags & 2097155) !== 0 ||  // opening for write
              (flags & 512)) {
            return ERRNO_CODES.EISDIR;
          }
        }
        return FS.nodePermissions(node, FS.flagsToPermissionString(flags));
      },MAX_OPEN_FDS:4096,nextfd:function (fd_start, fd_end) {
        fd_start = fd_start || 0;
        fd_end = fd_end || FS.MAX_OPEN_FDS;
        for (var fd = fd_start; fd <= fd_end; fd++) {
          if (!FS.streams[fd]) {
            return fd;
          }
        }
        throw new FS.ErrnoError(ERRNO_CODES.EMFILE);
      },getStream:function (fd) {
        return FS.streams[fd];
      },createStream:function (stream, fd_start, fd_end) {
        if (!FS.FSStream) {
          FS.FSStream = function(){};
          FS.FSStream.prototype = {};
          // compatibility
          Object.defineProperties(FS.FSStream.prototype, {
            object: {
              get: function() { return this.node; },
              set: function(val) { this.node = val; }
            },
            isRead: {
              get: function() { return (this.flags & 2097155) !== 1; }
            },
            isWrite: {
              get: function() { return (this.flags & 2097155) !== 0; }
            },
            isAppend: {
              get: function() { return (this.flags & 1024); }
            }
          });
        }
        // clone it, so we can return an instance of FSStream
        var newStream = new FS.FSStream();
        for (var p in stream) {
          newStream[p] = stream[p];
        }
        stream = newStream;
        var fd = FS.nextfd(fd_start, fd_end);
        stream.fd = fd;
        FS.streams[fd] = stream;
        return stream;
      },closeStream:function (fd) {
        FS.streams[fd] = null;
      },getStreamFromPtr:function (ptr) {
        return FS.streams[ptr - 1];
      },getPtrForStream:function (stream) {
        return stream ? stream.fd + 1 : 0;
      },chrdev_stream_ops:{open:function (stream) {
          var device = FS.getDevice(stream.node.rdev);
          // override node's stream ops with the device's
          stream.stream_ops = device.stream_ops;
          // forward the open call
          if (stream.stream_ops.open) {
            stream.stream_ops.open(stream);
          }
        },llseek:function () {
          throw new FS.ErrnoError(ERRNO_CODES.ESPIPE);
        }},major:function (dev) {
        return ((dev) >> 8);
      },minor:function (dev) {
        return ((dev) & 0xff);
      },makedev:function (ma, mi) {
        return ((ma) << 8 | (mi));
      },registerDevice:function (dev, ops) {
        FS.devices[dev] = { stream_ops: ops };
      },getDevice:function (dev) {
        return FS.devices[dev];
      },getMounts:function (mount) {
        var mounts = [];
        var check = [mount];
  
        while (check.length) {
          var m = check.pop();
  
          mounts.push(m);
  
          check.push.apply(check, m.mounts);
        }
  
        return mounts;
      },syncfs:function (populate, callback) {
        if (typeof(populate) === 'function') {
          callback = populate;
          populate = false;
        }
  
        var mounts = FS.getMounts(FS.root.mount);
        var completed = 0;
  
        function done(err) {
          if (err) {
            if (!done.errored) {
              done.errored = true;
              return callback(err);
            }
            return;
          }
          if (++completed >= mounts.length) {
            callback(null);
          }
        };
  
        // sync all mounts
        mounts.forEach(function (mount) {
          if (!mount.type.syncfs) {
            return done(null);
          }
          mount.type.syncfs(mount, populate, done);
        });
      },mount:function (type, opts, mountpoint) {
        var root = mountpoint === '/';
        var pseudo = !mountpoint;
        var node;
  
        if (root && FS.root) {
          throw new FS.ErrnoError(ERRNO_CODES.EBUSY);
        } else if (!root && !pseudo) {
          var lookup = FS.lookupPath(mountpoint, { follow_mount: false });
  
          mountpoint = lookup.path;  // use the absolute path
          node = lookup.node;
  
          if (FS.isMountpoint(node)) {
            throw new FS.ErrnoError(ERRNO_CODES.EBUSY);
          }
  
          if (!FS.isDir(node.mode)) {
            throw new FS.ErrnoError(ERRNO_CODES.ENOTDIR);
          }
        }
  
        var mount = {
          type: type,
          opts: opts,
          mountpoint: mountpoint,
          mounts: []
        };
  
        // create a root node for the fs
        var mountRoot = type.mount(mount);
        mountRoot.mount = mount;
        mount.root = mountRoot;
  
        if (root) {
          FS.root = mountRoot;
        } else if (node) {
          // set as a mountpoint
          node.mounted = mount;
  
          // add the new mount to the current mount's children
          if (node.mount) {
            node.mount.mounts.push(mount);
          }
        }
  
        return mountRoot;
      },unmount:function (mountpoint) {
        var lookup = FS.lookupPath(mountpoint, { follow_mount: false });
  
        if (!FS.isMountpoint(lookup.node)) {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
  
        // destroy the nodes for this mount, and all its child mounts
        var node = lookup.node;
        var mount = node.mounted;
        var mounts = FS.getMounts(mount);
  
        Object.keys(FS.nameTable).forEach(function (hash) {
          var current = FS.nameTable[hash];
  
          while (current) {
            var next = current.name_next;
  
            if (mounts.indexOf(current.mount) !== -1) {
              FS.destroyNode(current);
            }
  
            current = next;
          }
        });
  
        // no longer a mountpoint
        node.mounted = null;
  
        // remove this mount from the child mounts
        var idx = node.mount.mounts.indexOf(mount);
        assert(idx !== -1);
        node.mount.mounts.splice(idx, 1);
      },lookup:function (parent, name) {
        return parent.node_ops.lookup(parent, name);
      },mknod:function (path, mode, dev) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        var name = PATH.basename(path);
        var err = FS.mayCreate(parent, name);
        if (err) {
          throw new FS.ErrnoError(err);
        }
        if (!parent.node_ops.mknod) {
          throw new FS.ErrnoError(ERRNO_CODES.EPERM);
        }
        return parent.node_ops.mknod(parent, name, mode, dev);
      },create:function (path, mode) {
        mode = mode !== undefined ? mode : 438 /* 0666 */;
        mode &= 4095;
        mode |= 32768;
        return FS.mknod(path, mode, 0);
      },mkdir:function (path, mode) {
        mode = mode !== undefined ? mode : 511 /* 0777 */;
        mode &= 511 | 512;
        mode |= 16384;
        return FS.mknod(path, mode, 0);
      },mkdev:function (path, mode, dev) {
        if (typeof(dev) === 'undefined') {
          dev = mode;
          mode = 438 /* 0666 */;
        }
        mode |= 8192;
        return FS.mknod(path, mode, dev);
      },symlink:function (oldpath, newpath) {
        var lookup = FS.lookupPath(newpath, { parent: true });
        var parent = lookup.node;
        var newname = PATH.basename(newpath);
        var err = FS.mayCreate(parent, newname);
        if (err) {
          throw new FS.ErrnoError(err);
        }
        if (!parent.node_ops.symlink) {
          throw new FS.ErrnoError(ERRNO_CODES.EPERM);
        }
        return parent.node_ops.symlink(parent, newname, oldpath);
      },rename:function (old_path, new_path) {
        var old_dirname = PATH.dirname(old_path);
        var new_dirname = PATH.dirname(new_path);
        var old_name = PATH.basename(old_path);
        var new_name = PATH.basename(new_path);
        // parents must exist
        var lookup, old_dir, new_dir;
        try {
          lookup = FS.lookupPath(old_path, { parent: true });
          old_dir = lookup.node;
          lookup = FS.lookupPath(new_path, { parent: true });
          new_dir = lookup.node;
        } catch (e) {
          throw new FS.ErrnoError(ERRNO_CODES.EBUSY);
        }
        // need to be part of the same mount
        if (old_dir.mount !== new_dir.mount) {
          throw new FS.ErrnoError(ERRNO_CODES.EXDEV);
        }
        // source must exist
        var old_node = FS.lookupNode(old_dir, old_name);
        // old path should not be an ancestor of the new path
        var relative = PATH.relative(old_path, new_dirname);
        if (relative.charAt(0) !== '.') {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
        // new path should not be an ancestor of the old path
        relative = PATH.relative(new_path, old_dirname);
        if (relative.charAt(0) !== '.') {
          throw new FS.ErrnoError(ERRNO_CODES.ENOTEMPTY);
        }
        // see if the new path already exists
        var new_node;
        try {
          new_node = FS.lookupNode(new_dir, new_name);
        } catch (e) {
          // not fatal
        }
        // early out if nothing needs to change
        if (old_node === new_node) {
          return;
        }
        // we'll need to delete the old entry
        var isdir = FS.isDir(old_node.mode);
        var err = FS.mayDelete(old_dir, old_name, isdir);
        if (err) {
          throw new FS.ErrnoError(err);
        }
        // need delete permissions if we'll be overwriting.
        // need create permissions if new doesn't already exist.
        err = new_node ?
          FS.mayDelete(new_dir, new_name, isdir) :
          FS.mayCreate(new_dir, new_name);
        if (err) {
          throw new FS.ErrnoError(err);
        }
        if (!old_dir.node_ops.rename) {
          throw new FS.ErrnoError(ERRNO_CODES.EPERM);
        }
        if (FS.isMountpoint(old_node) || (new_node && FS.isMountpoint(new_node))) {
          throw new FS.ErrnoError(ERRNO_CODES.EBUSY);
        }
        // if we are going to change the parent, check write permissions
        if (new_dir !== old_dir) {
          err = FS.nodePermissions(old_dir, 'w');
          if (err) {
            throw new FS.ErrnoError(err);
          }
        }
        // remove the node from the lookup hash
        FS.hashRemoveNode(old_node);
        // do the underlying fs rename
        try {
          old_dir.node_ops.rename(old_node, new_dir, new_name);
        } catch (e) {
          throw e;
        } finally {
          // add the node back to the hash (in case node_ops.rename
          // changed its name)
          FS.hashAddNode(old_node);
        }
      },rmdir:function (path) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        var name = PATH.basename(path);
        var node = FS.lookupNode(parent, name);
        var err = FS.mayDelete(parent, name, true);
        if (err) {
          throw new FS.ErrnoError(err);
        }
        if (!parent.node_ops.rmdir) {
          throw new FS.ErrnoError(ERRNO_CODES.EPERM);
        }
        if (FS.isMountpoint(node)) {
          throw new FS.ErrnoError(ERRNO_CODES.EBUSY);
        }
        parent.node_ops.rmdir(parent, name);
        FS.destroyNode(node);
      },readdir:function (path) {
        var lookup = FS.lookupPath(path, { follow: true });
        var node = lookup.node;
        if (!node.node_ops.readdir) {
          throw new FS.ErrnoError(ERRNO_CODES.ENOTDIR);
        }
        return node.node_ops.readdir(node);
      },unlink:function (path) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        var name = PATH.basename(path);
        var node = FS.lookupNode(parent, name);
        var err = FS.mayDelete(parent, name, false);
        if (err) {
          // POSIX says unlink should set EPERM, not EISDIR
          if (err === ERRNO_CODES.EISDIR) err = ERRNO_CODES.EPERM;
          throw new FS.ErrnoError(err);
        }
        if (!parent.node_ops.unlink) {
          throw new FS.ErrnoError(ERRNO_CODES.EPERM);
        }
        if (FS.isMountpoint(node)) {
          throw new FS.ErrnoError(ERRNO_CODES.EBUSY);
        }
        parent.node_ops.unlink(parent, name);
        FS.destroyNode(node);
      },readlink:function (path) {
        var lookup = FS.lookupPath(path);
        var link = lookup.node;
        if (!link.node_ops.readlink) {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
        return link.node_ops.readlink(link);
      },stat:function (path, dontFollow) {
        var lookup = FS.lookupPath(path, { follow: !dontFollow });
        var node = lookup.node;
        if (!node.node_ops.getattr) {
          throw new FS.ErrnoError(ERRNO_CODES.EPERM);
        }
        return node.node_ops.getattr(node);
      },lstat:function (path) {
        return FS.stat(path, true);
      },chmod:function (path, mode, dontFollow) {
        var node;
        if (typeof path === 'string') {
          var lookup = FS.lookupPath(path, { follow: !dontFollow });
          node = lookup.node;
        } else {
          node = path;
        }
        if (!node.node_ops.setattr) {
          throw new FS.ErrnoError(ERRNO_CODES.EPERM);
        }
        node.node_ops.setattr(node, {
          mode: (mode & 4095) | (node.mode & ~4095),
          timestamp: Date.now()
        });
      },lchmod:function (path, mode) {
        FS.chmod(path, mode, true);
      },fchmod:function (fd, mode) {
        var stream = FS.getStream(fd);
        if (!stream) {
          throw new FS.ErrnoError(ERRNO_CODES.EBADF);
        }
        FS.chmod(stream.node, mode);
      },chown:function (path, uid, gid, dontFollow) {
        var node;
        if (typeof path === 'string') {
          var lookup = FS.lookupPath(path, { follow: !dontFollow });
          node = lookup.node;
        } else {
          node = path;
        }
        if (!node.node_ops.setattr) {
          throw new FS.ErrnoError(ERRNO_CODES.EPERM);
        }
        node.node_ops.setattr(node, {
          timestamp: Date.now()
          // we ignore the uid / gid for now
        });
      },lchown:function (path, uid, gid) {
        FS.chown(path, uid, gid, true);
      },fchown:function (fd, uid, gid) {
        var stream = FS.getStream(fd);
        if (!stream) {
          throw new FS.ErrnoError(ERRNO_CODES.EBADF);
        }
        FS.chown(stream.node, uid, gid);
      },truncate:function (path, len) {
        if (len < 0) {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
        var node;
        if (typeof path === 'string') {
          var lookup = FS.lookupPath(path, { follow: true });
          node = lookup.node;
        } else {
          node = path;
        }
        if (!node.node_ops.setattr) {
          throw new FS.ErrnoError(ERRNO_CODES.EPERM);
        }
        if (FS.isDir(node.mode)) {
          throw new FS.ErrnoError(ERRNO_CODES.EISDIR);
        }
        if (!FS.isFile(node.mode)) {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
        var err = FS.nodePermissions(node, 'w');
        if (err) {
          throw new FS.ErrnoError(err);
        }
        node.node_ops.setattr(node, {
          size: len,
          timestamp: Date.now()
        });
      },ftruncate:function (fd, len) {
        var stream = FS.getStream(fd);
        if (!stream) {
          throw new FS.ErrnoError(ERRNO_CODES.EBADF);
        }
        if ((stream.flags & 2097155) === 0) {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
        FS.truncate(stream.node, len);
      },utime:function (path, atime, mtime) {
        var lookup = FS.lookupPath(path, { follow: true });
        var node = lookup.node;
        node.node_ops.setattr(node, {
          timestamp: Math.max(atime, mtime)
        });
      },open:function (path, flags, mode, fd_start, fd_end) {
        flags = typeof flags === 'string' ? FS.modeStringToFlags(flags) : flags;
        mode = typeof mode === 'undefined' ? 438 /* 0666 */ : mode;
        if ((flags & 64)) {
          mode = (mode & 4095) | 32768;
        } else {
          mode = 0;
        }
        var node;
        if (typeof path === 'object') {
          node = path;
        } else {
          path = PATH.normalize(path);
          try {
            var lookup = FS.lookupPath(path, {
              follow: !(flags & 131072)
            });
            node = lookup.node;
          } catch (e) {
            // ignore
          }
        }
        // perhaps we need to create the node
        if ((flags & 64)) {
          if (node) {
            // if O_CREAT and O_EXCL are set, error out if the node already exists
            if ((flags & 128)) {
              throw new FS.ErrnoError(ERRNO_CODES.EEXIST);
            }
          } else {
            // node doesn't exist, try to create it
            node = FS.mknod(path, mode, 0);
          }
        }
        if (!node) {
          throw new FS.ErrnoError(ERRNO_CODES.ENOENT);
        }
        // can't truncate a device
        if (FS.isChrdev(node.mode)) {
          flags &= ~512;
        }
        // check permissions
        var err = FS.mayOpen(node, flags);
        if (err) {
          throw new FS.ErrnoError(err);
        }
        // do truncation if necessary
        if ((flags & 512)) {
          FS.truncate(node, 0);
        }
        // we've already handled these, don't pass down to the underlying vfs
        flags &= ~(128 | 512);
  
        // register the stream with the filesystem
        var stream = FS.createStream({
          node: node,
          path: FS.getPath(node),  // we want the absolute path to the node
          flags: flags,
          seekable: true,
          position: 0,
          stream_ops: node.stream_ops,
          // used by the file family libc calls (fopen, fwrite, ferror, etc.)
          ungotten: [],
          error: false
        }, fd_start, fd_end);
        // call the new stream's open function
        if (stream.stream_ops.open) {
          stream.stream_ops.open(stream);
        }
        if (Module['logReadFiles'] && !(flags & 1)) {
          if (!FS.readFiles) FS.readFiles = {};
          if (!(path in FS.readFiles)) {
            FS.readFiles[path] = 1;
            Module['printErr']('read file: ' + path);
          }
        }
        return stream;
      },close:function (stream) {
        try {
          if (stream.stream_ops.close) {
            stream.stream_ops.close(stream);
          }
        } catch (e) {
          throw e;
        } finally {
          FS.closeStream(stream.fd);
        }
      },llseek:function (stream, offset, whence) {
        if (!stream.seekable || !stream.stream_ops.llseek) {
          throw new FS.ErrnoError(ERRNO_CODES.ESPIPE);
        }
        return stream.stream_ops.llseek(stream, offset, whence);
      },read:function (stream, buffer, offset, length, position) {
        if (length < 0 || position < 0) {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
        if ((stream.flags & 2097155) === 1) {
          throw new FS.ErrnoError(ERRNO_CODES.EBADF);
        }
        if (FS.isDir(stream.node.mode)) {
          throw new FS.ErrnoError(ERRNO_CODES.EISDIR);
        }
        if (!stream.stream_ops.read) {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
        var seeking = true;
        if (typeof position === 'undefined') {
          position = stream.position;
          seeking = false;
        } else if (!stream.seekable) {
          throw new FS.ErrnoError(ERRNO_CODES.ESPIPE);
        }
        var bytesRead = stream.stream_ops.read(stream, buffer, offset, length, position);
        if (!seeking) stream.position += bytesRead;
        return bytesRead;
      },write:function (stream, buffer, offset, length, position, canOwn) {
        if (length < 0 || position < 0) {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
        if ((stream.flags & 2097155) === 0) {
          throw new FS.ErrnoError(ERRNO_CODES.EBADF);
        }
        if (FS.isDir(stream.node.mode)) {
          throw new FS.ErrnoError(ERRNO_CODES.EISDIR);
        }
        if (!stream.stream_ops.write) {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
        var seeking = true;
        if (typeof position === 'undefined') {
          position = stream.position;
          seeking = false;
        } else if (!stream.seekable) {
          throw new FS.ErrnoError(ERRNO_CODES.ESPIPE);
        }
        if (stream.flags & 1024) {
          // seek to the end before writing in append mode
          FS.llseek(stream, 0, 2);
        }
        var bytesWritten = stream.stream_ops.write(stream, buffer, offset, length, position, canOwn);
        if (!seeking) stream.position += bytesWritten;
        return bytesWritten;
      },allocate:function (stream, offset, length) {
        if (offset < 0 || length <= 0) {
          throw new FS.ErrnoError(ERRNO_CODES.EINVAL);
        }
        if ((stream.flags & 2097155) === 0) {
          throw new FS.ErrnoError(ERRNO_CODES.EBADF);
        }
        if (!FS.isFile(stream.node.mode) && !FS.isDir(node.mode)) {
          throw new FS.ErrnoError(ERRNO_CODES.ENODEV);
        }
        if (!stream.stream_ops.allocate) {
          throw new FS.ErrnoError(ERRNO_CODES.EOPNOTSUPP);
        }
        stream.stream_ops.allocate(stream, offset, length);
      },mmap:function (stream, buffer, offset, length, position, prot, flags) {
        // TODO if PROT is PROT_WRITE, make sure we have write access
        if ((stream.flags & 2097155) === 1) {
          throw new FS.ErrnoError(ERRNO_CODES.EACCES);
        }
        if (!stream.stream_ops.mmap) {
          throw new FS.ErrnoError(ERRNO_CODES.ENODEV);
        }
        return stream.stream_ops.mmap(stream, buffer, offset, length, position, prot, flags);
      },ioctl:function (stream, cmd, arg) {
        if (!stream.stream_ops.ioctl) {
          throw new FS.ErrnoError(ERRNO_CODES.ENOTTY);
        }
        return stream.stream_ops.ioctl(stream, cmd, arg);
      },readFile:function (path, opts) {
        opts = opts || {};
        opts.flags = opts.flags || 'r';
        opts.encoding = opts.encoding || 'binary';
        if (opts.encoding !== 'utf8' && opts.encoding !== 'binary') {
          throw new Error('Invalid encoding type "' + opts.encoding + '"');
        }
        var ret;
        var stream = FS.open(path, opts.flags);
        var stat = FS.stat(path);
        var length = stat.size;
        var buf = new Uint8Array(length);
        FS.read(stream, buf, 0, length, 0);
        if (opts.encoding === 'utf8') {
          ret = '';
          var utf8 = new Runtime.UTF8Processor();
          for (var i = 0; i < length; i++) {
            ret += utf8.processCChar(buf[i]);
          }
        } else if (opts.encoding === 'binary') {
          ret = buf;
        }
        FS.close(stream);
        return ret;
      },writeFile:function (path, data, opts) {
        opts = opts || {};
        opts.flags = opts.flags || 'w';
        opts.encoding = opts.encoding || 'utf8';
        if (opts.encoding !== 'utf8' && opts.encoding !== 'binary') {
          throw new Error('Invalid encoding type "' + opts.encoding + '"');
        }
        var stream = FS.open(path, opts.flags, opts.mode);
        if (opts.encoding === 'utf8') {
          var utf8 = new Runtime.UTF8Processor();
          var buf = new Uint8Array(utf8.processJSString(data));
          FS.write(stream, buf, 0, buf.length, 0, opts.canOwn);
        } else if (opts.encoding === 'binary') {
          FS.write(stream, data, 0, data.length, 0, opts.canOwn);
        }
        FS.close(stream);
      },cwd:function () {
        return FS.currentPath;
      },chdir:function (path) {
        var lookup = FS.lookupPath(path, { follow: true });
        if (!FS.isDir(lookup.node.mode)) {
          throw new FS.ErrnoError(ERRNO_CODES.ENOTDIR);
        }
        var err = FS.nodePermissions(lookup.node, 'x');
        if (err) {
          throw new FS.ErrnoError(err);
        }
        FS.currentPath = lookup.path;
      },createDefaultDirectories:function () {
        FS.mkdir('/tmp');
      },createDefaultDevices:function () {
        // create /dev
        FS.mkdir('/dev');
        // setup /dev/null
        FS.registerDevice(FS.makedev(1, 3), {
          read: function() { return 0; },
          write: function() { return 0; }
        });
        FS.mkdev('/dev/null', FS.makedev(1, 3));
        // setup /dev/tty and /dev/tty1
        // stderr needs to print output using Module['printErr']
        // so we register a second tty just for it.
        TTY.register(FS.makedev(5, 0), TTY.default_tty_ops);
        TTY.register(FS.makedev(6, 0), TTY.default_tty1_ops);
        FS.mkdev('/dev/tty', FS.makedev(5, 0));
        FS.mkdev('/dev/tty1', FS.makedev(6, 0));
        // we're not going to emulate the actual shm device,
        // just create the tmp dirs that reside in it commonly
        FS.mkdir('/dev/shm');
        FS.mkdir('/dev/shm/tmp');
      },createStandardStreams:function () {
        // TODO deprecate the old functionality of a single
        // input / output callback and that utilizes FS.createDevice
        // and instead require a unique set of stream ops
  
        // by default, we symlink the standard streams to the
        // default tty devices. however, if the standard streams
        // have been overwritten we create a unique device for
        // them instead.
        if (Module['stdin']) {
          FS.createDevice('/dev', 'stdin', Module['stdin']);
        } else {
          FS.symlink('/dev/tty', '/dev/stdin');
        }
        if (Module['stdout']) {
          FS.createDevice('/dev', 'stdout', null, Module['stdout']);
        } else {
          FS.symlink('/dev/tty', '/dev/stdout');
        }
        if (Module['stderr']) {
          FS.createDevice('/dev', 'stderr', null, Module['stderr']);
        } else {
          FS.symlink('/dev/tty1', '/dev/stderr');
        }
  
        // open default streams for the stdin, stdout and stderr devices
        var stdin = FS.open('/dev/stdin', 'r');
        HEAP32[((_stdin)>>2)]=FS.getPtrForStream(stdin);
        assert(stdin.fd === 0, 'invalid handle for stdin (' + stdin.fd + ')');
  
        var stdout = FS.open('/dev/stdout', 'w');
        HEAP32[((_stdout)>>2)]=FS.getPtrForStream(stdout);
        assert(stdout.fd === 1, 'invalid handle for stdout (' + stdout.fd + ')');
  
        var stderr = FS.open('/dev/stderr', 'w');
        HEAP32[((_stderr)>>2)]=FS.getPtrForStream(stderr);
        assert(stderr.fd === 2, 'invalid handle for stderr (' + stderr.fd + ')');
      },ensureErrnoError:function () {
        if (FS.ErrnoError) return;
        FS.ErrnoError = function ErrnoError(errno) {
          this.errno = errno;
          for (var key in ERRNO_CODES) {
            if (ERRNO_CODES[key] === errno) {
              this.code = key;
              break;
            }
          }
          this.message = ERRNO_MESSAGES[errno];
          if (this.stack) this.stack = demangleAll(this.stack);
        };
        FS.ErrnoError.prototype = new Error();
        FS.ErrnoError.prototype.constructor = FS.ErrnoError;
        // Some errors may happen quite a bit, to avoid overhead we reuse them (and suffer a lack of stack info)
        [ERRNO_CODES.ENOENT].forEach(function(code) {
          FS.genericErrors[code] = new FS.ErrnoError(code);
          FS.genericErrors[code].stack = '<generic error, no stack>';
        });
      },staticInit:function () {
        FS.ensureErrnoError();
  
        FS.nameTable = new Array(4096);
  
        FS.mount(MEMFS, {}, '/');
  
        FS.createDefaultDirectories();
        FS.createDefaultDevices();
      },init:function (input, output, error) {
        assert(!FS.init.initialized, 'FS.init was previously called. If you want to initialize later with custom parameters, remove any earlier calls (note that one is automatically added to the generated code)');
        FS.init.initialized = true;
  
        FS.ensureErrnoError();
  
        // Allow Module.stdin etc. to provide defaults, if none explicitly passed to us here
        Module['stdin'] = input || Module['stdin'];
        Module['stdout'] = output || Module['stdout'];
        Module['stderr'] = error || Module['stderr'];
  
        FS.createStandardStreams();
      },quit:function () {
        FS.init.initialized = false;
        for (var i = 0; i < FS.streams.length; i++) {
          var stream = FS.streams[i];
          if (!stream) {
            continue;
          }
          FS.close(stream);
        }
      },getMode:function (canRead, canWrite) {
        var mode = 0;
        if (canRead) mode |= 292 | 73;
        if (canWrite) mode |= 146;
        return mode;
      },joinPath:function (parts, forceRelative) {
        var path = PATH.join.apply(null, parts);
        if (forceRelative && path[0] == '/') path = path.substr(1);
        return path;
      },absolutePath:function (relative, base) {
        return PATH.resolve(base, relative);
      },standardizePath:function (path) {
        return PATH.normalize(path);
      },findObject:function (path, dontResolveLastLink) {
        var ret = FS.analyzePath(path, dontResolveLastLink);
        if (ret.exists) {
          return ret.object;
        } else {
          ___setErrNo(ret.error);
          return null;
        }
      },analyzePath:function (path, dontResolveLastLink) {
        // operate from within the context of the symlink's target
        try {
          var lookup = FS.lookupPath(path, { follow: !dontResolveLastLink });
          path = lookup.path;
        } catch (e) {
        }
        var ret = {
          isRoot: false, exists: false, error: 0, name: null, path: null, object: null,
          parentExists: false, parentPath: null, parentObject: null
        };
        try {
          var lookup = FS.lookupPath(path, { parent: true });
          ret.parentExists = true;
          ret.parentPath = lookup.path;
          ret.parentObject = lookup.node;
          ret.name = PATH.basename(path);
          lookup = FS.lookupPath(path, { follow: !dontResolveLastLink });
          ret.exists = true;
          ret.path = lookup.path;
          ret.object = lookup.node;
          ret.name = lookup.node.name;
          ret.isRoot = lookup.path === '/';
        } catch (e) {
          ret.error = e.errno;
        };
        return ret;
      },createFolder:function (parent, name, canRead, canWrite) {
        var path = PATH.join2(typeof parent === 'string' ? parent : FS.getPath(parent), name);
        var mode = FS.getMode(canRead, canWrite);
        return FS.mkdir(path, mode);
      },createPath:function (parent, path, canRead, canWrite) {
        parent = typeof parent === 'string' ? parent : FS.getPath(parent);
        var parts = path.split('/').reverse();
        while (parts.length) {
          var part = parts.pop();
          if (!part) continue;
          var current = PATH.join2(parent, part);
          try {
            FS.mkdir(current);
          } catch (e) {
            // ignore EEXIST
          }
          parent = current;
        }
        return current;
      },createFile:function (parent, name, properties, canRead, canWrite) {
        var path = PATH.join2(typeof parent === 'string' ? parent : FS.getPath(parent), name);
        var mode = FS.getMode(canRead, canWrite);
        return FS.create(path, mode);
      },createDataFile:function (parent, name, data, canRead, canWrite, canOwn) {
        var path = name ? PATH.join2(typeof parent === 'string' ? parent : FS.getPath(parent), name) : parent;
        var mode = FS.getMode(canRead, canWrite);
        var node = FS.create(path, mode);
        if (data) {
          if (typeof data === 'string') {
            var arr = new Array(data.length);
            for (var i = 0, len = data.length; i < len; ++i) arr[i] = data.charCodeAt(i);
            data = arr;
          }
          // make sure we can write to the file
          FS.chmod(node, mode | 146);
          var stream = FS.open(node, 'w');
          FS.write(stream, data, 0, data.length, 0, canOwn);
          FS.close(stream);
          FS.chmod(node, mode);
        }
        return node;
      },createDevice:function (parent, name, input, output) {
        var path = PATH.join2(typeof parent === 'string' ? parent : FS.getPath(parent), name);
        var mode = FS.getMode(!!input, !!output);
        if (!FS.createDevice.major) FS.createDevice.major = 64;
        var dev = FS.makedev(FS.createDevice.major++, 0);
        // Create a fake device that a set of stream ops to emulate
        // the old behavior.
        FS.registerDevice(dev, {
          open: function(stream) {
            stream.seekable = false;
          },
          close: function(stream) {
            // flush any pending line data
            if (output && output.buffer && output.buffer.length) {
              output(10);
            }
          },
          read: function(stream, buffer, offset, length, pos /* ignored */) {
            var bytesRead = 0;
            for (var i = 0; i < length; i++) {
              var result;
              try {
                result = input();
              } catch (e) {
                throw new FS.ErrnoError(ERRNO_CODES.EIO);
              }
              if (result === undefined && bytesRead === 0) {
                throw new FS.ErrnoError(ERRNO_CODES.EAGAIN);
              }
              if (result === null || result === undefined) break;
              bytesRead++;
              buffer[offset+i] = result;
            }
            if (bytesRead) {
              stream.node.timestamp = Date.now();
            }
            return bytesRead;
          },
          write: function(stream, buffer, offset, length, pos) {
            for (var i = 0; i < length; i++) {
              try {
                output(buffer[offset+i]);
              } catch (e) {
                throw new FS.ErrnoError(ERRNO_CODES.EIO);
              }
            }
            if (length) {
              stream.node.timestamp = Date.now();
            }
            return i;
          }
        });
        return FS.mkdev(path, mode, dev);
      },createLink:function (parent, name, target, canRead, canWrite) {
        var path = PATH.join2(typeof parent === 'string' ? parent : FS.getPath(parent), name);
        return FS.symlink(target, path);
      },forceLoadFile:function (obj) {
        if (obj.isDevice || obj.isFolder || obj.link || obj.contents) return true;
        var success = true;
        if (typeof XMLHttpRequest !== 'undefined') {
          throw new Error("Lazy loading should have been performed (contents set) in createLazyFile, but it was not. Lazy loading only works in web workers. Use --embed-file or --preload-file in emcc on the main thread.");
        } else if (Module['read']) {
          // Command-line.
          try {
            // WARNING: Can't read binary files in V8's d8 or tracemonkey's js, as
            //          read() will try to parse UTF8.
            obj.contents = intArrayFromString(Module['read'](obj.url), true);
          } catch (e) {
            success = false;
          }
        } else {
          throw new Error('Cannot load without read() or XMLHttpRequest.');
        }
        if (!success) ___setErrNo(ERRNO_CODES.EIO);
        return success;
      },createLazyFile:function (parent, name, url, canRead, canWrite) {
        // Lazy chunked Uint8Array (implements get and length from Uint8Array). Actual getting is abstracted away for eventual reuse.
        function LazyUint8Array() {
          this.lengthKnown = false;
          this.chunks = []; // Loaded chunks. Index is the chunk number
        }
        LazyUint8Array.prototype.get = function LazyUint8Array_get(idx) {
          if (idx > this.length-1 || idx < 0) {
            return undefined;
          }
          var chunkOffset = idx % this.chunkSize;
          var chunkNum = Math.floor(idx / this.chunkSize);
          return this.getter(chunkNum)[chunkOffset];
        }
        LazyUint8Array.prototype.setDataGetter = function LazyUint8Array_setDataGetter(getter) {
          this.getter = getter;
        }
        LazyUint8Array.prototype.cacheLength = function LazyUint8Array_cacheLength() {
            // Find length
            var xhr = new XMLHttpRequest();
            xhr.open('HEAD', url, false);
            xhr.send(null);
            if (!(xhr.status >= 200 && xhr.status < 300 || xhr.status === 304)) throw new Error("Couldn't load " + url + ". Status: " + xhr.status);
            var datalength = Number(xhr.getResponseHeader("Content-length"));
            var header;
            var hasByteServing = (header = xhr.getResponseHeader("Accept-Ranges")) && header === "bytes";
            var chunkSize = 1024*1024; // Chunk size in bytes
  
            if (!hasByteServing) chunkSize = datalength;
  
            // Function to get a range from the remote URL.
            var doXHR = (function(from, to) {
              if (from > to) throw new Error("invalid range (" + from + ", " + to + ") or no bytes requested!");
              if (to > datalength-1) throw new Error("only " + datalength + " bytes available! programmer error!");
  
              // TODO: Use mozResponseArrayBuffer, responseStream, etc. if available.
              var xhr = new XMLHttpRequest();
              xhr.open('GET', url, false);
              if (datalength !== chunkSize) xhr.setRequestHeader("Range", "bytes=" + from + "-" + to);
  
              // Some hints to the browser that we want binary data.
              if (typeof Uint8Array != 'undefined') xhr.responseType = 'arraybuffer';
              if (xhr.overrideMimeType) {
                xhr.overrideMimeType('text/plain; charset=x-user-defined');
              }
  
              xhr.send(null);
              if (!(xhr.status >= 200 && xhr.status < 300 || xhr.status === 304)) throw new Error("Couldn't load " + url + ". Status: " + xhr.status);
              if (xhr.response !== undefined) {
                return new Uint8Array(xhr.response || []);
              } else {
                return intArrayFromString(xhr.responseText || '', true);
              }
            });
            var lazyArray = this;
            lazyArray.setDataGetter(function(chunkNum) {
              var start = chunkNum * chunkSize;
              var end = (chunkNum+1) * chunkSize - 1; // including this byte
              end = Math.min(end, datalength-1); // if datalength-1 is selected, this is the last block
              if (typeof(lazyArray.chunks[chunkNum]) === "undefined") {
                lazyArray.chunks[chunkNum] = doXHR(start, end);
              }
              if (typeof(lazyArray.chunks[chunkNum]) === "undefined") throw new Error("doXHR failed!");
              return lazyArray.chunks[chunkNum];
            });
  
            this._length = datalength;
            this._chunkSize = chunkSize;
            this.lengthKnown = true;
        }
        if (typeof XMLHttpRequest !== 'undefined') {
          if (!ENVIRONMENT_IS_WORKER) throw 'Cannot do synchronous binary XHRs outside webworkers in modern browsers. Use --embed-file or --preload-file in emcc';
          var lazyArray = new LazyUint8Array();
          Object.defineProperty(lazyArray, "length", {
              get: function() {
                  if(!this.lengthKnown) {
                      this.cacheLength();
                  }
                  return this._length;
              }
          });
          Object.defineProperty(lazyArray, "chunkSize", {
              get: function() {
                  if(!this.lengthKnown) {
                      this.cacheLength();
                  }
                  return this._chunkSize;
              }
          });
  
          var properties = { isDevice: false, contents: lazyArray };
        } else {
          var properties = { isDevice: false, url: url };
        }
  
        var node = FS.createFile(parent, name, properties, canRead, canWrite);
        // This is a total hack, but I want to get this lazy file code out of the
        // core of MEMFS. If we want to keep this lazy file concept I feel it should
        // be its own thin LAZYFS proxying calls to MEMFS.
        if (properties.contents) {
          node.contents = properties.contents;
        } else if (properties.url) {
          node.contents = null;
          node.url = properties.url;
        }
        // override each stream op with one that tries to force load the lazy file first
        var stream_ops = {};
        var keys = Object.keys(node.stream_ops);
        keys.forEach(function(key) {
          var fn = node.stream_ops[key];
          stream_ops[key] = function forceLoadLazyFile() {
            if (!FS.forceLoadFile(node)) {
              throw new FS.ErrnoError(ERRNO_CODES.EIO);
            }
            return fn.apply(null, arguments);
          };
        });
        // use a custom read function
        stream_ops.read = function stream_ops_read(stream, buffer, offset, length, position) {
          if (!FS.forceLoadFile(node)) {
            throw new FS.ErrnoError(ERRNO_CODES.EIO);
          }
          var contents = stream.node.contents;
          if (position >= contents.length)
            return 0;
          var size = Math.min(contents.length - position, length);
          assert(size >= 0);
          if (contents.slice) { // normal array
            for (var i = 0; i < size; i++) {
              buffer[offset + i] = contents[position + i];
            }
          } else {
            for (var i = 0; i < size; i++) { // LazyUint8Array from sync binary XHR
              buffer[offset + i] = contents.get(position + i);
            }
          }
          return size;
        };
        node.stream_ops = stream_ops;
        return node;
      },createPreloadedFile:function (parent, name, url, canRead, canWrite, onload, onerror, dontCreateFile, canOwn) {
        Browser.init();
        // TODO we should allow people to just pass in a complete filename instead
        // of parent and name being that we just join them anyways
        var fullname = name ? PATH.resolve(PATH.join2(parent, name)) : parent;
        function processData(byteArray) {
          function finish(byteArray) {
            if (!dontCreateFile) {
              FS.createDataFile(parent, name, byteArray, canRead, canWrite, canOwn);
            }
            if (onload) onload();
            removeRunDependency('cp ' + fullname);
          }
          var handled = false;
          Module['preloadPlugins'].forEach(function(plugin) {
            if (handled) return;
            if (plugin['canHandle'](fullname)) {
              plugin['handle'](byteArray, fullname, finish, function() {
                if (onerror) onerror();
                removeRunDependency('cp ' + fullname);
              });
              handled = true;
            }
          });
          if (!handled) finish(byteArray);
        }
        addRunDependency('cp ' + fullname);
        if (typeof url == 'string') {
          Browser.asyncLoad(url, function(byteArray) {
            processData(byteArray);
          }, onerror);
        } else {
          processData(url);
        }
      },indexedDB:function () {
        return window.indexedDB || window.mozIndexedDB || window.webkitIndexedDB || window.msIndexedDB;
      },DB_NAME:function () {
        return 'EM_FS_' + window.location.pathname;
      },DB_VERSION:20,DB_STORE_NAME:"FILE_DATA",saveFilesToDB:function (paths, onload, onerror) {
        onload = onload || function(){};
        onerror = onerror || function(){};
        var indexedDB = FS.indexedDB();
        try {
          var openRequest = indexedDB.open(FS.DB_NAME(), FS.DB_VERSION);
        } catch (e) {
          return onerror(e);
        }
        openRequest.onupgradeneeded = function openRequest_onupgradeneeded() {
          console.log('creating db');
          var db = openRequest.result;
          db.createObjectStore(FS.DB_STORE_NAME);
        };
        openRequest.onsuccess = function openRequest_onsuccess() {
          var db = openRequest.result;
          var transaction = db.transaction([FS.DB_STORE_NAME], 'readwrite');
          var files = transaction.objectStore(FS.DB_STORE_NAME);
          var ok = 0, fail = 0, total = paths.length;
          function finish() {
            if (fail == 0) onload(); else onerror();
          }
          paths.forEach(function(path) {
            var putRequest = files.put(FS.analyzePath(path).object.contents, path);
            putRequest.onsuccess = function putRequest_onsuccess() { ok++; if (ok + fail == total) finish() };
            putRequest.onerror = function putRequest_onerror() { fail++; if (ok + fail == total) finish() };
          });
          transaction.onerror = onerror;
        };
        openRequest.onerror = onerror;
      },loadFilesFromDB:function (paths, onload, onerror) {
        onload = onload || function(){};
        onerror = onerror || function(){};
        var indexedDB = FS.indexedDB();
        try {
          var openRequest = indexedDB.open(FS.DB_NAME(), FS.DB_VERSION);
        } catch (e) {
          return onerror(e);
        }
        openRequest.onupgradeneeded = onerror; // no database to load from
        openRequest.onsuccess = function openRequest_onsuccess() {
          var db = openRequest.result;
          try {
            var transaction = db.transaction([FS.DB_STORE_NAME], 'readonly');
          } catch(e) {
            onerror(e);
            return;
          }
          var files = transaction.objectStore(FS.DB_STORE_NAME);
          var ok = 0, fail = 0, total = paths.length;
          function finish() {
            if (fail == 0) onload(); else onerror();
          }
          paths.forEach(function(path) {
            var getRequest = files.get(path);
            getRequest.onsuccess = function getRequest_onsuccess() {
              if (FS.analyzePath(path).exists) {
                FS.unlink(path);
              }
              FS.createDataFile(PATH.dirname(path), PATH.basename(path), getRequest.result, true, true, true);
              ok++;
              if (ok + fail == total) finish();
            };
            getRequest.onerror = function getRequest_onerror() { fail++; if (ok + fail == total) finish() };
          });
          transaction.onerror = onerror;
        };
        openRequest.onerror = onerror;
      }};var PATH={splitPath:function (filename) {
        var splitPathRe = /^(\/?|)([\s\S]*?)((?:\.{1,2}|[^\/]+?|)(\.[^.\/]*|))(?:[\/]*)$/;
        return splitPathRe.exec(filename).slice(1);
      },normalizeArray:function (parts, allowAboveRoot) {
        // if the path tries to go above the root, `up` ends up > 0
        var up = 0;
        for (var i = parts.length - 1; i >= 0; i--) {
          var last = parts[i];
          if (last === '.') {
            parts.splice(i, 1);
          } else if (last === '..') {
            parts.splice(i, 1);
            up++;
          } else if (up) {
            parts.splice(i, 1);
            up--;
          }
        }
        // if the path is allowed to go above the root, restore leading ..s
        if (allowAboveRoot) {
          for (; up--; up) {
            parts.unshift('..');
          }
        }
        return parts;
      },normalize:function (path) {
        var isAbsolute = path.charAt(0) === '/',
            trailingSlash = path.substr(-1) === '/';
        // Normalize the path
        path = PATH.normalizeArray(path.split('/').filter(function(p) {
          return !!p;
        }), !isAbsolute).join('/');
        if (!path && !isAbsolute) {
          path = '.';
        }
        if (path && trailingSlash) {
          path += '/';
        }
        return (isAbsolute ? '/' : '') + path;
      },dirname:function (path) {
        var result = PATH.splitPath(path),
            root = result[0],
            dir = result[1];
        if (!root && !dir) {
          // No dirname whatsoever
          return '.';
        }
        if (dir) {
          // It has a dirname, strip trailing slash
          dir = dir.substr(0, dir.length - 1);
        }
        return root + dir;
      },basename:function (path) {
        // EMSCRIPTEN return '/'' for '/', not an empty string
        if (path === '/') return '/';
        var lastSlash = path.lastIndexOf('/');
        if (lastSlash === -1) return path;
        return path.substr(lastSlash+1);
      },extname:function (path) {
        return PATH.splitPath(path)[3];
      },join:function () {
        var paths = Array.prototype.slice.call(arguments, 0);
        return PATH.normalize(paths.join('/'));
      },join2:function (l, r) {
        return PATH.normalize(l + '/' + r);
      },resolve:function () {
        var resolvedPath = '',
          resolvedAbsolute = false;
        for (var i = arguments.length - 1; i >= -1 && !resolvedAbsolute; i--) {
          var path = (i >= 0) ? arguments[i] : FS.cwd();
          // Skip empty and invalid entries
          if (typeof path !== 'string') {
            throw new TypeError('Arguments to path.resolve must be strings');
          } else if (!path) {
            continue;
          }
          resolvedPath = path + '/' + resolvedPath;
          resolvedAbsolute = path.charAt(0) === '/';
        }
        // At this point the path should be resolved to a full absolute path, but
        // handle relative paths to be safe (might happen when process.cwd() fails)
        resolvedPath = PATH.normalizeArray(resolvedPath.split('/').filter(function(p) {
          return !!p;
        }), !resolvedAbsolute).join('/');
        return ((resolvedAbsolute ? '/' : '') + resolvedPath) || '.';
      },relative:function (from, to) {
        from = PATH.resolve(from).substr(1);
        to = PATH.resolve(to).substr(1);
        function trim(arr) {
          var start = 0;
          for (; start < arr.length; start++) {
            if (arr[start] !== '') break;
          }
          var end = arr.length - 1;
          for (; end >= 0; end--) {
            if (arr[end] !== '') break;
          }
          if (start > end) return [];
          return arr.slice(start, end - start + 1);
        }
        var fromParts = trim(from.split('/'));
        var toParts = trim(to.split('/'));
        var length = Math.min(fromParts.length, toParts.length);
        var samePartsLength = length;
        for (var i = 0; i < length; i++) {
          if (fromParts[i] !== toParts[i]) {
            samePartsLength = i;
            break;
          }
        }
        var outputParts = [];
        for (var i = samePartsLength; i < fromParts.length; i++) {
          outputParts.push('..');
        }
        outputParts = outputParts.concat(toParts.slice(samePartsLength));
        return outputParts.join('/');
      }};var Browser={mainLoop:{scheduler:null,method:"",shouldPause:false,paused:false,queue:[],pause:function () {
          Browser.mainLoop.shouldPause = true;
        },resume:function () {
          if (Browser.mainLoop.paused) {
            Browser.mainLoop.paused = false;
            Browser.mainLoop.scheduler();
          }
          Browser.mainLoop.shouldPause = false;
        },updateStatus:function () {
          if (Module['setStatus']) {
            var message = Module['statusMessage'] || 'Please wait...';
            var remaining = Browser.mainLoop.remainingBlockers;
            var expected = Browser.mainLoop.expectedBlockers;
            if (remaining) {
              if (remaining < expected) {
                Module['setStatus'](message + ' (' + (expected - remaining) + '/' + expected + ')');
              } else {
                Module['setStatus'](message);
              }
            } else {
              Module['setStatus']('');
            }
          }
        }},isFullScreen:false,pointerLock:false,moduleContextCreatedCallbacks:[],workers:[],init:function () {
        if (!Module["preloadPlugins"]) Module["preloadPlugins"] = []; // needs to exist even in workers
  
        if (Browser.initted || ENVIRONMENT_IS_WORKER) return;
        Browser.initted = true;
  
        try {
          new Blob();
          Browser.hasBlobConstructor = true;
        } catch(e) {
          Browser.hasBlobConstructor = false;
          console.log("warning: no blob constructor, cannot create blobs with mimetypes");
        }
        Browser.BlobBuilder = typeof MozBlobBuilder != "undefined" ? MozBlobBuilder : (typeof WebKitBlobBuilder != "undefined" ? WebKitBlobBuilder : (!Browser.hasBlobConstructor ? console.log("warning: no BlobBuilder") : null));
        Browser.URLObject = typeof window != "undefined" ? (window.URL ? window.URL : window.webkitURL) : undefined;
        if (!Module.noImageDecoding && typeof Browser.URLObject === 'undefined') {
          console.log("warning: Browser does not support creating object URLs. Built-in browser image decoding will not be available.");
          Module.noImageDecoding = true;
        }
  
        // Support for plugins that can process preloaded files. You can add more of these to
        // your app by creating and appending to Module.preloadPlugins.
        //
        // Each plugin is asked if it can handle a file based on the file's name. If it can,
        // it is given the file's raw data. When it is done, it calls a callback with the file's
        // (possibly modified) data. For example, a plugin might decompress a file, or it
        // might create some side data structure for use later (like an Image element, etc.).
  
        var imagePlugin = {};
        imagePlugin['canHandle'] = function imagePlugin_canHandle(name) {
          return !Module.noImageDecoding && /\.(jpg|jpeg|png|bmp)$/i.test(name);
        };
        imagePlugin['handle'] = function imagePlugin_handle(byteArray, name, onload, onerror) {
          var b = null;
          if (Browser.hasBlobConstructor) {
            try {
              b = new Blob([byteArray], { type: Browser.getMimetype(name) });
              if (b.size !== byteArray.length) { // Safari bug #118630
                // Safari's Blob can only take an ArrayBuffer
                b = new Blob([(new Uint8Array(byteArray)).buffer], { type: Browser.getMimetype(name) });
              }
            } catch(e) {
              Runtime.warnOnce('Blob constructor present but fails: ' + e + '; falling back to blob builder');
            }
          }
          if (!b) {
            var bb = new Browser.BlobBuilder();
            bb.append((new Uint8Array(byteArray)).buffer); // we need to pass a buffer, and must copy the array to get the right data range
            b = bb.getBlob();
          }
          var url = Browser.URLObject.createObjectURL(b);
          assert(typeof url == 'string', 'createObjectURL must return a url as a string');
          var img = new Image();
          img.onload = function img_onload() {
            assert(img.complete, 'Image ' + name + ' could not be decoded');
            var canvas = document.createElement('canvas');
            canvas.width = img.width;
            canvas.height = img.height;
            var ctx = canvas.getContext('2d');
            ctx.drawImage(img, 0, 0);
            Module["preloadedImages"][name] = canvas;
            Browser.URLObject.revokeObjectURL(url);
            if (onload) onload(byteArray);
          };
          img.onerror = function img_onerror(event) {
            console.log('Image ' + url + ' could not be decoded');
            if (onerror) onerror();
          };
          img.src = url;
        };
        Module['preloadPlugins'].push(imagePlugin);
  
        var audioPlugin = {};
        audioPlugin['canHandle'] = function audioPlugin_canHandle(name) {
          return !Module.noAudioDecoding && name.substr(-4) in { '.ogg': 1, '.wav': 1, '.mp3': 1 };
        };
        audioPlugin['handle'] = function audioPlugin_handle(byteArray, name, onload, onerror) {
          var done = false;
          function finish(audio) {
            if (done) return;
            done = true;
            Module["preloadedAudios"][name] = audio;
            if (onload) onload(byteArray);
          }
          function fail() {
            if (done) return;
            done = true;
            Module["preloadedAudios"][name] = new Audio(); // empty shim
            if (onerror) onerror();
          }
          if (Browser.hasBlobConstructor) {
            try {
              var b = new Blob([byteArray], { type: Browser.getMimetype(name) });
            } catch(e) {
              return fail();
            }
            var url = Browser.URLObject.createObjectURL(b); // XXX we never revoke this!
            assert(typeof url == 'string', 'createObjectURL must return a url as a string');
            var audio = new Audio();
            audio.addEventListener('canplaythrough', function() { finish(audio) }, false); // use addEventListener due to chromium bug 124926
            audio.onerror = function audio_onerror(event) {
              if (done) return;
              console.log('warning: browser could not fully decode audio ' + name + ', trying slower base64 approach');
              function encode64(data) {
                var BASE = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
                var PAD = '=';
                var ret = '';
                var leftchar = 0;
                var leftbits = 0;
                for (var i = 0; i < data.length; i++) {
                  leftchar = (leftchar << 8) | data[i];
                  leftbits += 8;
                  while (leftbits >= 6) {
                    var curr = (leftchar >> (leftbits-6)) & 0x3f;
                    leftbits -= 6;
                    ret += BASE[curr];
                  }
                }
                if (leftbits == 2) {
                  ret += BASE[(leftchar&3) << 4];
                  ret += PAD + PAD;
                } else if (leftbits == 4) {
                  ret += BASE[(leftchar&0xf) << 2];
                  ret += PAD;
                }
                return ret;
              }
              audio.src = 'data:audio/x-' + name.substr(-3) + ';base64,' + encode64(byteArray);
              finish(audio); // we don't wait for confirmation this worked - but it's worth trying
            };
            audio.src = url;
            // workaround for chrome bug 124926 - we do not always get oncanplaythrough or onerror
            Browser.safeSetTimeout(function() {
              finish(audio); // try to use it even though it is not necessarily ready to play
            }, 10000);
          } else {
            return fail();
          }
        };
        Module['preloadPlugins'].push(audioPlugin);
  
        // Canvas event setup
  
        var canvas = Module['canvas'];
        
        // forced aspect ratio can be enabled by defining 'forcedAspectRatio' on Module
        // Module['forcedAspectRatio'] = 4 / 3;
        
        canvas.requestPointerLock = canvas['requestPointerLock'] ||
                                    canvas['mozRequestPointerLock'] ||
                                    canvas['webkitRequestPointerLock'] ||
                                    canvas['msRequestPointerLock'] ||
                                    function(){};
        canvas.exitPointerLock = document['exitPointerLock'] ||
                                 document['mozExitPointerLock'] ||
                                 document['webkitExitPointerLock'] ||
                                 document['msExitPointerLock'] ||
                                 function(){}; // no-op if function does not exist
        canvas.exitPointerLock = canvas.exitPointerLock.bind(document);
  
        function pointerLockChange() {
          Browser.pointerLock = document['pointerLockElement'] === canvas ||
                                document['mozPointerLockElement'] === canvas ||
                                document['webkitPointerLockElement'] === canvas ||
                                document['msPointerLockElement'] === canvas;
        }
  
        document.addEventListener('pointerlockchange', pointerLockChange, false);
        document.addEventListener('mozpointerlockchange', pointerLockChange, false);
        document.addEventListener('webkitpointerlockchange', pointerLockChange, false);
        document.addEventListener('mspointerlockchange', pointerLockChange, false);
  
        if (Module['elementPointerLock']) {
          canvas.addEventListener("click", function(ev) {
            if (!Browser.pointerLock && canvas.requestPointerLock) {
              canvas.requestPointerLock();
              ev.preventDefault();
            }
          }, false);
        }
      },createContext:function (canvas, useWebGL, setInModule, webGLContextAttributes) {
        var ctx;
        var errorInfo = '?';
        function onContextCreationError(event) {
          errorInfo = event.statusMessage || errorInfo;
        }
        try {
          if (useWebGL) {
            var contextAttributes = {
              antialias: false,
              alpha: false
            };
  
            if (webGLContextAttributes) {
              for (var attribute in webGLContextAttributes) {
                contextAttributes[attribute] = webGLContextAttributes[attribute];
              }
            }
  
  
            canvas.addEventListener('webglcontextcreationerror', onContextCreationError, false);
            try {
              ['experimental-webgl', 'webgl'].some(function(webglId) {
                return ctx = canvas.getContext(webglId, contextAttributes);
              });
            } finally {
              canvas.removeEventListener('webglcontextcreationerror', onContextCreationError, false);
            }
          } else {
            ctx = canvas.getContext('2d');
          }
          if (!ctx) throw ':(';
        } catch (e) {
          Module.print('Could not create canvas: ' + [errorInfo, e]);
          return null;
        }
        if (useWebGL) {
          // Set the background of the WebGL canvas to black
          canvas.style.backgroundColor = "black";
  
          // Warn on context loss
          canvas.addEventListener('webglcontextlost', function(event) {
            alert('WebGL context lost. You will need to reload the page.');
          }, false);
        }
        if (setInModule) {
          GLctx = Module.ctx = ctx;
          Module.useWebGL = useWebGL;
          Browser.moduleContextCreatedCallbacks.forEach(function(callback) { callback() });
          Browser.init();
        }
        return ctx;
      },destroyContext:function (canvas, useWebGL, setInModule) {},fullScreenHandlersInstalled:false,lockPointer:undefined,resizeCanvas:undefined,requestFullScreen:function (lockPointer, resizeCanvas) {
        Browser.lockPointer = lockPointer;
        Browser.resizeCanvas = resizeCanvas;
        if (typeof Browser.lockPointer === 'undefined') Browser.lockPointer = true;
        if (typeof Browser.resizeCanvas === 'undefined') Browser.resizeCanvas = false;
  
        var canvas = Module['canvas'];
        function fullScreenChange() {
          Browser.isFullScreen = false;
          var canvasContainer = canvas.parentNode;
          if ((document['webkitFullScreenElement'] || document['webkitFullscreenElement'] ||
               document['mozFullScreenElement'] || document['mozFullscreenElement'] ||
               document['fullScreenElement'] || document['fullscreenElement'] ||
               document['msFullScreenElement'] || document['msFullscreenElement'] ||
               document['webkitCurrentFullScreenElement']) === canvasContainer) {
            canvas.cancelFullScreen = document['cancelFullScreen'] ||
                                      document['mozCancelFullScreen'] ||
                                      document['webkitCancelFullScreen'] ||
                                      document['msExitFullscreen'] ||
                                      document['exitFullscreen'] ||
                                      function() {};
            canvas.cancelFullScreen = canvas.cancelFullScreen.bind(document);
            if (Browser.lockPointer) canvas.requestPointerLock();
            Browser.isFullScreen = true;
            if (Browser.resizeCanvas) Browser.setFullScreenCanvasSize();
          } else {
            
            // remove the full screen specific parent of the canvas again to restore the HTML structure from before going full screen
            canvasContainer.parentNode.insertBefore(canvas, canvasContainer);
            canvasContainer.parentNode.removeChild(canvasContainer);
            
            if (Browser.resizeCanvas) Browser.setWindowedCanvasSize();
          }
          if (Module['onFullScreen']) Module['onFullScreen'](Browser.isFullScreen);
          Browser.updateCanvasDimensions(canvas);
        }
  
        if (!Browser.fullScreenHandlersInstalled) {
          Browser.fullScreenHandlersInstalled = true;
          document.addEventListener('fullscreenchange', fullScreenChange, false);
          document.addEventListener('mozfullscreenchange', fullScreenChange, false);
          document.addEventListener('webkitfullscreenchange', fullScreenChange, false);
          document.addEventListener('MSFullscreenChange', fullScreenChange, false);
        }
  
        // create a new parent to ensure the canvas has no siblings. this allows browsers to optimize full screen performance when its parent is the full screen root
        var canvasContainer = document.createElement("div");
        canvas.parentNode.insertBefore(canvasContainer, canvas);
        canvasContainer.appendChild(canvas);
        
        // use parent of canvas as full screen root to allow aspect ratio correction (Firefox stretches the root to screen size)
        canvasContainer.requestFullScreen = canvasContainer['requestFullScreen'] ||
                                            canvasContainer['mozRequestFullScreen'] ||
                                            canvasContainer['msRequestFullscreen'] ||
                                           (canvasContainer['webkitRequestFullScreen'] ? function() { canvasContainer['webkitRequestFullScreen'](Element['ALLOW_KEYBOARD_INPUT']) } : null);
        canvasContainer.requestFullScreen();
      },requestAnimationFrame:function requestAnimationFrame(func) {
        if (typeof window === 'undefined') { // Provide fallback to setTimeout if window is undefined (e.g. in Node.js)
          setTimeout(func, 1000/60);
        } else {
          if (!window.requestAnimationFrame) {
            window.requestAnimationFrame = window['requestAnimationFrame'] ||
                                           window['mozRequestAnimationFrame'] ||
                                           window['webkitRequestAnimationFrame'] ||
                                           window['msRequestAnimationFrame'] ||
                                           window['oRequestAnimationFrame'] ||
                                           window['setTimeout'];
          }
          window.requestAnimationFrame(func);
        }
      },safeCallback:function (func) {
        return function() {
          if (!ABORT) return func.apply(null, arguments);
        };
      },safeRequestAnimationFrame:function (func) {
        return Browser.requestAnimationFrame(function() {
          if (!ABORT) func();
        });
      },safeSetTimeout:function (func, timeout) {
        return setTimeout(function() {
          if (!ABORT) func();
        }, timeout);
      },safeSetInterval:function (func, timeout) {
        return setInterval(function() {
          if (!ABORT) func();
        }, timeout);
      },getMimetype:function (name) {
        return {
          'jpg': 'image/jpeg',
          'jpeg': 'image/jpeg',
          'png': 'image/png',
          'bmp': 'image/bmp',
          'ogg': 'audio/ogg',
          'wav': 'audio/wav',
          'mp3': 'audio/mpeg'
        }[name.substr(name.lastIndexOf('.')+1)];
      },getUserMedia:function (func) {
        if(!window.getUserMedia) {
          window.getUserMedia = navigator['getUserMedia'] ||
                                navigator['mozGetUserMedia'];
        }
        window.getUserMedia(func);
      },getMovementX:function (event) {
        return event['movementX'] ||
               event['mozMovementX'] ||
               event['webkitMovementX'] ||
               0;
      },getMovementY:function (event) {
        return event['movementY'] ||
               event['mozMovementY'] ||
               event['webkitMovementY'] ||
               0;
      },getMouseWheelDelta:function (event) {
        return Math.max(-1, Math.min(1, event.type === 'DOMMouseScroll' ? event.detail : -event.wheelDelta));
      },mouseX:0,mouseY:0,mouseMovementX:0,mouseMovementY:0,touches:{},lastTouches:{},calculateMouseEvent:function (event) { // event should be mousemove, mousedown or mouseup
        if (Browser.pointerLock) {
          // When the pointer is locked, calculate the coordinates
          // based on the movement of the mouse.
          // Workaround for Firefox bug 764498
          if (event.type != 'mousemove' &&
              ('mozMovementX' in event)) {
            Browser.mouseMovementX = Browser.mouseMovementY = 0;
          } else {
            Browser.mouseMovementX = Browser.getMovementX(event);
            Browser.mouseMovementY = Browser.getMovementY(event);
          }
          
          // check if SDL is available
          if (typeof SDL != "undefined") {
          	Browser.mouseX = SDL.mouseX + Browser.mouseMovementX;
          	Browser.mouseY = SDL.mouseY + Browser.mouseMovementY;
          } else {
          	// just add the mouse delta to the current absolut mouse position
          	// FIXME: ideally this should be clamped against the canvas size and zero
          	Browser.mouseX += Browser.mouseMovementX;
          	Browser.mouseY += Browser.mouseMovementY;
          }        
        } else {
          // Otherwise, calculate the movement based on the changes
          // in the coordinates.
          var rect = Module["canvas"].getBoundingClientRect();
          var cw = Module["canvas"].width;
          var ch = Module["canvas"].height;
  
          // Neither .scrollX or .pageXOffset are defined in a spec, but
          // we prefer .scrollX because it is currently in a spec draft.
          // (see: http://www.w3.org/TR/2013/WD-cssom-view-20131217/)
          var scrollX = ((typeof window.scrollX !== 'undefined') ? window.scrollX : window.pageXOffset);
          var scrollY = ((typeof window.scrollY !== 'undefined') ? window.scrollY : window.pageYOffset);
          // If this assert lands, it's likely because the browser doesn't support scrollX or pageXOffset
          // and we have no viable fallback.
          assert((typeof scrollX !== 'undefined') && (typeof scrollY !== 'undefined'), 'Unable to retrieve scroll position, mouse positions likely broken.');
  
          if (event.type === 'touchstart' || event.type === 'touchend' || event.type === 'touchmove') {
            var touch = event.touch;
            if (touch === undefined) {
              return; // the "touch" property is only defined in SDL
  
            }
            var adjustedX = touch.pageX - (scrollX + rect.left);
            var adjustedY = touch.pageY - (scrollY + rect.top);
  
            adjustedX = adjustedX * (cw / rect.width);
            adjustedY = adjustedY * (ch / rect.height);
  
            var coords = { x: adjustedX, y: adjustedY };
            
            if (event.type === 'touchstart') {
              Browser.lastTouches[touch.identifier] = coords;
              Browser.touches[touch.identifier] = coords;
            } else if (event.type === 'touchend' || event.type === 'touchmove') {
              Browser.lastTouches[touch.identifier] = Browser.touches[touch.identifier];
              Browser.touches[touch.identifier] = { x: adjustedX, y: adjustedY };
            } 
            return;
          }
  
          var x = event.pageX - (scrollX + rect.left);
          var y = event.pageY - (scrollY + rect.top);
  
          // the canvas might be CSS-scaled compared to its backbuffer;
          // SDL-using content will want mouse coordinates in terms
          // of backbuffer units.
          x = x * (cw / rect.width);
          y = y * (ch / rect.height);
  
          Browser.mouseMovementX = x - Browser.mouseX;
          Browser.mouseMovementY = y - Browser.mouseY;
          Browser.mouseX = x;
          Browser.mouseY = y;
        }
      },xhrLoad:function (url, onload, onerror) {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', url, true);
        xhr.responseType = 'arraybuffer';
        xhr.onload = function xhr_onload() {
          if (xhr.status == 200 || (xhr.status == 0 && xhr.response)) { // file URLs can return 0
            onload(xhr.response);
          } else {
            onerror();
          }
        };
        xhr.onerror = onerror;
        xhr.send(null);
      },asyncLoad:function (url, onload, onerror, noRunDep) {
        Browser.xhrLoad(url, function(arrayBuffer) {
          assert(arrayBuffer, 'Loading data file "' + url + '" failed (no arrayBuffer).');
          onload(new Uint8Array(arrayBuffer));
          if (!noRunDep) removeRunDependency('al ' + url);
        }, function(event) {
          if (onerror) {
            onerror();
          } else {
            throw 'Loading data file "' + url + '" failed.';
          }
        });
        if (!noRunDep) addRunDependency('al ' + url);
      },resizeListeners:[],updateResizeListeners:function () {
        var canvas = Module['canvas'];
        Browser.resizeListeners.forEach(function(listener) {
          listener(canvas.width, canvas.height);
        });
      },setCanvasSize:function (width, height, noUpdates) {
        var canvas = Module['canvas'];
        Browser.updateCanvasDimensions(canvas, width, height);
        if (!noUpdates) Browser.updateResizeListeners();
      },windowedWidth:0,windowedHeight:0,setFullScreenCanvasSize:function () {
        // check if SDL is available   
        if (typeof SDL != "undefined") {
        	var flags = HEAPU32[((SDL.screen+Runtime.QUANTUM_SIZE*0)>>2)];
        	flags = flags | 0x00800000; // set SDL_FULLSCREEN flag
        	HEAP32[((SDL.screen+Runtime.QUANTUM_SIZE*0)>>2)]=flags
        }
        Browser.updateResizeListeners();
      },setWindowedCanvasSize:function () {
        // check if SDL is available       
        if (typeof SDL != "undefined") {
        	var flags = HEAPU32[((SDL.screen+Runtime.QUANTUM_SIZE*0)>>2)];
        	flags = flags & ~0x00800000; // clear SDL_FULLSCREEN flag
        	HEAP32[((SDL.screen+Runtime.QUANTUM_SIZE*0)>>2)]=flags
        }
        Browser.updateResizeListeners();
      },updateCanvasDimensions:function (canvas, wNative, hNative) {
        if (wNative && hNative) {
          canvas.widthNative = wNative;
          canvas.heightNative = hNative;
        } else {
          wNative = canvas.widthNative;
          hNative = canvas.heightNative;
        }
        var w = wNative;
        var h = hNative;
        if (Module['forcedAspectRatio'] && Module['forcedAspectRatio'] > 0) {
          if (w/h < Module['forcedAspectRatio']) {
            w = Math.round(h * Module['forcedAspectRatio']);
          } else {
            h = Math.round(w / Module['forcedAspectRatio']);
          }
        }
        if (((document['webkitFullScreenElement'] || document['webkitFullscreenElement'] ||
             document['mozFullScreenElement'] || document['mozFullscreenElement'] ||
             document['fullScreenElement'] || document['fullscreenElement'] ||
             document['msFullScreenElement'] || document['msFullscreenElement'] ||
             document['webkitCurrentFullScreenElement']) === canvas.parentNode) && (typeof screen != 'undefined')) {
           var factor = Math.min(screen.width / w, screen.height / h);
           w = Math.round(w * factor);
           h = Math.round(h * factor);
        }
        if (Browser.resizeCanvas) {
          if (canvas.width  != w) canvas.width  = w;
          if (canvas.height != h) canvas.height = h;
          if (typeof canvas.style != 'undefined') {
            canvas.style.removeProperty( "width");
            canvas.style.removeProperty("height");
          }
        } else {
          if (canvas.width  != wNative) canvas.width  = wNative;
          if (canvas.height != hNative) canvas.height = hNative;
          if (typeof canvas.style != 'undefined') {
            if (w != wNative || h != hNative) {
              canvas.style.setProperty( "width", w + "px", "important");
              canvas.style.setProperty("height", h + "px", "important");
            } else {
              canvas.style.removeProperty( "width");
              canvas.style.removeProperty("height");
            }
          }
        }
      }};

  function _time(ptr) {
      var ret = Math.floor(Date.now()/1000);
      if (ptr) {
        HEAP32[((ptr)>>2)]=ret;
      }
      return ret;
    }

   
  Module["_strlen"] = _strlen;

  
  function _emscripten_memcpy_big(dest, src, num) {
      HEAPU8.set(HEAPU8.subarray(src, src+num), dest);
      return dest;
    } 
  Module["_memcpy"] = _memcpy;
___errno_state = Runtime.staticAlloc(4); HEAP32[((___errno_state)>>2)]=0;
Module["requestFullScreen"] = function Module_requestFullScreen(lockPointer, resizeCanvas) { Browser.requestFullScreen(lockPointer, resizeCanvas) };
  Module["requestAnimationFrame"] = function Module_requestAnimationFrame(func) { Browser.requestAnimationFrame(func) };
  Module["setCanvasSize"] = function Module_setCanvasSize(width, height, noUpdates) { Browser.setCanvasSize(width, height, noUpdates) };
  Module["pauseMainLoop"] = function Module_pauseMainLoop() { Browser.mainLoop.pause() };
  Module["resumeMainLoop"] = function Module_resumeMainLoop() { Browser.mainLoop.resume() };
  Module["getUserMedia"] = function Module_getUserMedia() { Browser.getUserMedia() }
FS.staticInit();__ATINIT__.unshift({ func: function() { if (!Module["noFSInit"] && !FS.init.initialized) FS.init() } });__ATMAIN__.push({ func: function() { FS.ignorePermissions = false } });__ATEXIT__.push({ func: function() { FS.quit() } });Module["FS_createFolder"] = FS.createFolder;Module["FS_createPath"] = FS.createPath;Module["FS_createDataFile"] = FS.createDataFile;Module["FS_createPreloadedFile"] = FS.createPreloadedFile;Module["FS_createLazyFile"] = FS.createLazyFile;Module["FS_createLink"] = FS.createLink;Module["FS_createDevice"] = FS.createDevice;
__ATINIT__.unshift({ func: function() { TTY.init() } });__ATEXIT__.push({ func: function() { TTY.shutdown() } });TTY.utf8 = new Runtime.UTF8Processor();
if (ENVIRONMENT_IS_NODE) { var fs = require("fs"); NODEFS.staticInit(); }
STACK_BASE = STACKTOP = Runtime.alignMemory(STATICTOP);

staticSealed = true; // seal the static portion of memory

STACK_MAX = STACK_BASE + 5242880;

DYNAMIC_BASE = DYNAMICTOP = Runtime.alignMemory(STACK_MAX);

assert(DYNAMIC_BASE < TOTAL_MEMORY, "TOTAL_MEMORY not big enough for stack");


var Math_min = Math.min;
function nullFunc_iiii(x) { Module["printErr"]("Invalid function pointer called with signature 'iiii'. Perhaps this is an invalid value (e.g. caused by calling a virtual method on a NULL pointer)? Or calling a function with an incorrect type, which will fail? (it is worth building your source files with -Werror (warnings are errors), as warnings can indicate undefined behavior which can cause this)");  Module["printErr"]("Build with ASSERTIONS=2 for more info."); abort(x) }

function nullFunc_vii(x) { Module["printErr"]("Invalid function pointer called with signature 'vii'. Perhaps this is an invalid value (e.g. caused by calling a virtual method on a NULL pointer)? Or calling a function with an incorrect type, which will fail? (it is worth building your source files with -Werror (warnings are errors), as warnings can indicate undefined behavior which can cause this)");  Module["printErr"]("Build with ASSERTIONS=2 for more info."); abort(x) }

function invoke_iiii(index,a1,a2,a3) {
  try {
    return Module["dynCall_iiii"](index,a1,a2,a3);
  } catch(e) {
    if (typeof e !== 'number' && e !== 'longjmp') throw e;
    asm["setThrew"](1, 0);
  }
}

function invoke_vii(index,a1,a2) {
  try {
    Module["dynCall_vii"](index,a1,a2);
  } catch(e) {
    if (typeof e !== 'number' && e !== 'longjmp') throw e;
    asm["setThrew"](1, 0);
  }
}

function asmPrintInt(x, y) {
  Module.print('int ' + x + ',' + y);// + ' ' + new Error().stack);
}
function asmPrintFloat(x, y) {
  Module.print('float ' + x + ',' + y);// + ' ' + new Error().stack);
}
// EMSCRIPTEN_START_ASM
var asm = (function(global, env, buffer) {
  'almost asm';
  var HEAP8 = new global.Int8Array(buffer);
  var HEAP16 = new global.Int16Array(buffer);
  var HEAP32 = new global.Int32Array(buffer);
  var HEAPU8 = new global.Uint8Array(buffer);
  var HEAPU16 = new global.Uint16Array(buffer);
  var HEAPU32 = new global.Uint32Array(buffer);
  var HEAPF32 = new global.Float32Array(buffer);
  var HEAPF64 = new global.Float64Array(buffer);

  var STACKTOP=env.STACKTOP|0;
  var STACK_MAX=env.STACK_MAX|0;
  var tempDoublePtr=env.tempDoublePtr|0;
  var ABORT=env.ABORT|0;

  var __THREW__ = 0;
  var threwValue = 0;
  var setjmpId = 0;
  var undef = 0;
  var nan = +env.NaN, inf = +env.Infinity;
  var tempInt = 0, tempBigInt = 0, tempBigIntP = 0, tempBigIntS = 0, tempBigIntR = 0.0, tempBigIntI = 0, tempBigIntD = 0, tempValue = 0, tempDouble = 0.0;

  var tempRet0 = 0;
  var tempRet1 = 0;
  var tempRet2 = 0;
  var tempRet3 = 0;
  var tempRet4 = 0;
  var tempRet5 = 0;
  var tempRet6 = 0;
  var tempRet7 = 0;
  var tempRet8 = 0;
  var tempRet9 = 0;
  var Math_floor=global.Math.floor;
  var Math_abs=global.Math.abs;
  var Math_sqrt=global.Math.sqrt;
  var Math_pow=global.Math.pow;
  var Math_cos=global.Math.cos;
  var Math_sin=global.Math.sin;
  var Math_tan=global.Math.tan;
  var Math_acos=global.Math.acos;
  var Math_asin=global.Math.asin;
  var Math_atan=global.Math.atan;
  var Math_atan2=global.Math.atan2;
  var Math_exp=global.Math.exp;
  var Math_log=global.Math.log;
  var Math_ceil=global.Math.ceil;
  var Math_imul=global.Math.imul;
  var abort=env.abort;
  var assert=env.assert;
  var asmPrintInt=env.asmPrintInt;
  var asmPrintFloat=env.asmPrintFloat;
  var Math_min=env.min;
  var nullFunc_iiii=env.nullFunc_iiii;
  var nullFunc_vii=env.nullFunc_vii;
  var invoke_iiii=env.invoke_iiii;
  var invoke_vii=env.invoke_vii;
  var _fflush=env._fflush;
  var _abort=env._abort;
  var ___setErrNo=env.___setErrNo;
  var _sbrk=env._sbrk;
  var _time=env._time;
  var _emscripten_memcpy_big=env._emscripten_memcpy_big;
  var _sysconf=env._sysconf;
  var ___errno_location=env.___errno_location;
  var tempFloat = 0.0;

// EMSCRIPTEN_START_FUNCS
function stackAlloc(size) {
  size = size|0;
  var ret = 0;
  ret = STACKTOP;
  STACKTOP = (STACKTOP + size)|0;
STACKTOP = (STACKTOP + 7)&-8;
  return ret|0;
}
function stackSave() {
  return STACKTOP|0;
}
function stackRestore(top) {
  top = top|0;
  STACKTOP = top;
}
function setThrew(threw, value) {
  threw = threw|0;
  value = value|0;
  if ((__THREW__|0) == 0) {
    __THREW__ = threw;
    threwValue = value;
  }
}
function copyTempFloat(ptr) {
  ptr = ptr|0;
  HEAP8[tempDoublePtr] = HEAP8[ptr];
  HEAP8[tempDoublePtr+1|0] = HEAP8[ptr+1|0];
  HEAP8[tempDoublePtr+2|0] = HEAP8[ptr+2|0];
  HEAP8[tempDoublePtr+3|0] = HEAP8[ptr+3|0];
}
function copyTempDouble(ptr) {
  ptr = ptr|0;
  HEAP8[tempDoublePtr] = HEAP8[ptr];
  HEAP8[tempDoublePtr+1|0] = HEAP8[ptr+1|0];
  HEAP8[tempDoublePtr+2|0] = HEAP8[ptr+2|0];
  HEAP8[tempDoublePtr+3|0] = HEAP8[ptr+3|0];
  HEAP8[tempDoublePtr+4|0] = HEAP8[ptr+4|0];
  HEAP8[tempDoublePtr+5|0] = HEAP8[ptr+5|0];
  HEAP8[tempDoublePtr+6|0] = HEAP8[ptr+6|0];
  HEAP8[tempDoublePtr+7|0] = HEAP8[ptr+7|0];
}

function setTempRet0(value) {
  value = value|0;
  tempRet0 = value;
}

function setTempRet1(value) {
  value = value|0;
  tempRet1 = value;
}

function setTempRet2(value) {
  value = value|0;
  tempRet2 = value;
}

function setTempRet3(value) {
  value = value|0;
  tempRet3 = value;
}

function setTempRet4(value) {
  value = value|0;
  tempRet4 = value;
}

function setTempRet5(value) {
  value = value|0;
  tempRet5 = value;
}

function setTempRet6(value) {
  value = value|0;
  tempRet6 = value;
}

function setTempRet7(value) {
  value = value|0;
  tempRet7 = value;
}

function setTempRet8(value) {
  value = value|0;
  tempRet8 = value;
}

function setTempRet9(value) {
  value = value|0;
  tempRet9 = value;
}

function _zcalloc($opaque,$items,$size) {
 $opaque = $opaque|0;
 $items = $items|0;
 $size = $size|0;
 var $0 = 0, $1 = 0, $10 = 0, $11 = 0, $12 = 0, $13 = 0, $2 = 0, $3 = 0, $4 = 0, $5 = 0, $6 = 0, $7 = 0, $8 = 0, $9 = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 16|0;
 $0 = $opaque;
 $1 = $items;
 $2 = $size;
 $3 = $0;
 $4 = ($3|0)!=(0|0);
 if ($4) {
  $5 = $2;
  $6 = $2;
  $7 = (($5) - ($6))|0;
  $8 = $1;
  $9 = (($8) + ($7))|0;
  $1 = $9;
 }
 $10 = $1;
 $11 = $2;
 $12 = Math_imul($10, $11)|0;
 $13 = (_malloc($12)|0);
 STACKTOP = sp;return ($13|0);
}
function _zcfree($opaque,$ptr) {
 $opaque = $opaque|0;
 $ptr = $ptr|0;
 var $0 = 0, $1 = 0, $2 = 0, $3 = 0, $4 = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 16|0;
 $0 = $opaque;
 $1 = $ptr;
 $2 = $1;
 _free($2);
 $3 = $0;
 $4 = ($3|0)!=(0|0);
 if ($4) {
 }
 STACKTOP = sp;return;
}
function _inflate_table($type,$lens,$codes,$table,$bits,$work) {
 $type = $type|0;
 $lens = $lens|0;
 $codes = $codes|0;
 $table = $table|0;
 $bits = $bits|0;
 $work = $work|0;
 var $0 = 0, $1 = 0, $10 = 0, $100 = 0, $101 = 0, $102 = 0, $103 = 0, $104 = 0, $105 = 0, $106 = 0, $107 = 0, $108 = 0, $109 = 0, $11 = 0, $110 = 0, $111 = 0, $112 = 0, $113 = 0, $114 = 0, $115 = 0;
 var $116 = 0, $117 = 0, $118 = 0, $119 = 0, $12 = 0, $120 = 0, $121 = 0, $122 = 0, $123 = 0, $124 = 0, $125 = 0, $126 = 0, $127 = 0, $128 = 0, $129 = 0, $13 = 0, $130 = 0, $131 = 0, $132 = 0, $133 = 0;
 var $134 = 0, $135 = 0, $136 = 0, $137 = 0, $138 = 0, $139 = 0, $14 = 0, $140 = 0, $141 = 0, $142 = 0, $143 = 0, $144 = 0, $145 = 0, $146 = 0, $147 = 0, $148 = 0, $149 = 0, $15 = 0, $150 = 0, $151 = 0;
 var $152 = 0, $153 = 0, $154 = 0, $155 = 0, $156 = 0, $157 = 0, $158 = 0, $159 = 0, $16 = 0, $160 = 0, $161 = 0, $162 = 0, $163 = 0, $164 = 0, $165 = 0, $166 = 0, $167 = 0, $168 = 0, $169 = 0, $17 = 0;
 var $170 = 0, $171 = 0, $172 = 0, $173 = 0, $174 = 0, $175 = 0, $176 = 0, $177 = 0, $178 = 0, $179 = 0, $18 = 0, $180 = 0, $181 = 0, $182 = 0, $183 = 0, $184 = 0, $185 = 0, $186 = 0, $187 = 0, $188 = 0;
 var $189 = 0, $19 = 0, $190 = 0, $191 = 0, $192 = 0, $193 = 0, $194 = 0, $195 = 0, $196 = 0, $197 = 0, $198 = 0, $199 = 0, $2 = 0, $20 = 0, $200 = 0, $201 = 0, $202 = 0, $203 = 0, $204 = 0, $205 = 0;
 var $206 = 0, $207 = 0, $208 = 0, $209 = 0, $21 = 0, $210 = 0, $211 = 0, $212 = 0, $213 = 0, $214 = 0, $215 = 0, $216 = 0, $217 = 0, $218 = 0, $219 = 0, $22 = 0, $220 = 0, $221 = 0, $222 = 0, $223 = 0;
 var $224 = 0, $225 = 0, $226 = 0, $227 = 0, $228 = 0, $229 = 0, $23 = 0, $230 = 0, $231 = 0, $232 = 0, $233 = 0, $234 = 0, $235 = 0, $236 = 0, $237 = 0, $238 = 0, $239 = 0, $24 = 0, $240 = 0, $241 = 0;
 var $242 = 0, $243 = 0, $244 = 0, $245 = 0, $246 = 0, $247 = 0, $248 = 0, $249 = 0, $25 = 0, $250 = 0, $251 = 0, $252 = 0, $253 = 0, $254 = 0, $255 = 0, $256 = 0, $257 = 0, $258 = 0, $259 = 0, $26 = 0;
 var $260 = 0, $261 = 0, $262 = 0, $263 = 0, $264 = 0, $265 = 0, $266 = 0, $267 = 0, $268 = 0, $269 = 0, $27 = 0, $270 = 0, $271 = 0, $272 = 0, $273 = 0, $274 = 0, $275 = 0, $276 = 0, $277 = 0, $278 = 0;
 var $279 = 0, $28 = 0, $280 = 0, $281 = 0, $282 = 0, $283 = 0, $284 = 0, $285 = 0, $286 = 0, $287 = 0, $288 = 0, $289 = 0, $29 = 0, $290 = 0, $291 = 0, $292 = 0, $293 = 0, $294 = 0, $295 = 0, $296 = 0;
 var $297 = 0, $298 = 0, $299 = 0, $3 = 0, $30 = 0, $300 = 0, $301 = 0, $302 = 0, $303 = 0, $304 = 0, $305 = 0, $306 = 0, $307 = 0, $308 = 0, $309 = 0, $31 = 0, $310 = 0, $311 = 0, $312 = 0, $313 = 0;
 var $314 = 0, $315 = 0, $316 = 0, $317 = 0, $318 = 0, $319 = 0, $32 = 0, $320 = 0, $321 = 0, $322 = 0, $323 = 0, $324 = 0, $325 = 0, $326 = 0, $327 = 0, $328 = 0, $329 = 0, $33 = 0, $330 = 0, $331 = 0;
 var $332 = 0, $333 = 0, $334 = 0, $335 = 0, $336 = 0, $337 = 0, $338 = 0, $339 = 0, $34 = 0, $340 = 0, $341 = 0, $342 = 0, $343 = 0, $344 = 0, $345 = 0, $346 = 0, $35 = 0, $36 = 0, $37 = 0, $38 = 0;
 var $39 = 0, $4 = 0, $40 = 0, $41 = 0, $42 = 0, $43 = 0, $44 = 0, $45 = 0, $46 = 0, $47 = 0, $48 = 0, $49 = 0, $5 = 0, $50 = 0, $51 = 0, $52 = 0, $53 = 0, $54 = 0, $55 = 0, $56 = 0;
 var $57 = 0, $58 = 0, $59 = 0, $6 = 0, $60 = 0, $61 = 0, $62 = 0, $63 = 0, $64 = 0, $65 = 0, $66 = 0, $67 = 0, $68 = 0, $69 = 0, $7 = 0, $70 = 0, $71 = 0, $72 = 0, $73 = 0, $74 = 0;
 var $75 = 0, $76 = 0, $77 = 0, $78 = 0, $79 = 0, $8 = 0, $80 = 0, $81 = 0, $82 = 0, $83 = 0, $84 = 0, $85 = 0, $86 = 0, $87 = 0, $88 = 0, $89 = 0, $9 = 0, $90 = 0, $91 = 0, $92 = 0;
 var $93 = 0, $94 = 0, $95 = 0, $96 = 0, $97 = 0, $98 = 0, $99 = 0, $base = 0, $count = 0, $curr = 0, $drop = 0, $end = 0, $extra = 0, $fill = 0, $here = 0, $huff = 0, $incr = 0, $left = 0, $len = 0, $low = 0;
 var $mask = 0, $max = 0, $min = 0, $next = 0, $offs = 0, $root = 0, $sym = 0, $used = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 176|0;
 $here = sp + 168|0;
 $count = sp + 104|0;
 $offs = sp + 136|0;
 $1 = $type;
 $2 = $lens;
 $3 = $codes;
 $4 = $table;
 $5 = $bits;
 $6 = $work;
 $len = 0;
 while(1) {
  $7 = $len;
  $8 = ($7>>>0)<=(15);
  if (!($8)) {
   break;
  }
  $9 = $len;
  $10 = (($count) + ($9<<1)|0);
  HEAP16[$10>>1] = 0;
  $11 = $len;
  $12 = (($11) + 1)|0;
  $len = $12;
 }
 $sym = 0;
 while(1) {
  $13 = $sym;
  $14 = $3;
  $15 = ($13>>>0)<($14>>>0);
  if (!($15)) {
   break;
  }
  $16 = $sym;
  $17 = $2;
  $18 = (($17) + ($16<<1)|0);
  $19 = HEAP16[$18>>1]|0;
  $20 = $19&65535;
  $21 = (($count) + ($20<<1)|0);
  $22 = HEAP16[$21>>1]|0;
  $23 = (($22) + 1)<<16>>16;
  HEAP16[$21>>1] = $23;
  $24 = $sym;
  $25 = (($24) + 1)|0;
  $sym = $25;
 }
 $26 = $5;
 $27 = HEAP32[$26>>2]|0;
 $root = $27;
 $max = 15;
 while(1) {
  $28 = $max;
  $29 = ($28>>>0)>=(1);
  if (!($29)) {
   break;
  }
  $30 = $max;
  $31 = (($count) + ($30<<1)|0);
  $32 = HEAP16[$31>>1]|0;
  $33 = $32&65535;
  $34 = ($33|0)!=(0);
  if ($34) {
   label = 12;
   break;
  }
  $35 = $max;
  $36 = (($35) + -1)|0;
  $max = $36;
 }
 if ((label|0) == 12) {
 }
 $37 = $root;
 $38 = $max;
 $39 = ($37>>>0)>($38>>>0);
 if ($39) {
  $40 = $max;
  $root = $40;
 }
 $41 = $max;
 $42 = ($41|0)==(0);
 if ($42) {
  HEAP8[$here] = 64;
  $43 = (($here) + 1|0);
  HEAP8[$43] = 1;
  $44 = (($here) + 2|0);
  HEAP16[$44>>1] = 0;
  $45 = $4;
  $46 = HEAP32[$45>>2]|0;
  $47 = (($46) + 4|0);
  HEAP32[$45>>2] = $47;
  ;HEAP16[$46+0>>1]=HEAP16[$here+0>>1]|0;HEAP16[$46+2>>1]=HEAP16[$here+2>>1]|0;
  $48 = $4;
  $49 = HEAP32[$48>>2]|0;
  $50 = (($49) + 4|0);
  HEAP32[$48>>2] = $50;
  ;HEAP16[$49+0>>1]=HEAP16[$here+0>>1]|0;HEAP16[$49+2>>1]=HEAP16[$here+2>>1]|0;
  $51 = $5;
  HEAP32[$51>>2] = 1;
  $0 = 0;
  $346 = $0;
  STACKTOP = sp;return ($346|0);
 }
 $min = 1;
 while(1) {
  $52 = $min;
  $53 = $max;
  $54 = ($52>>>0)<($53>>>0);
  if (!($54)) {
   break;
  }
  $55 = $min;
  $56 = (($count) + ($55<<1)|0);
  $57 = HEAP16[$56>>1]|0;
  $58 = $57&65535;
  $59 = ($58|0)!=(0);
  if ($59) {
   label = 22;
   break;
  }
  $60 = $min;
  $61 = (($60) + 1)|0;
  $min = $61;
 }
 if ((label|0) == 22) {
 }
 $62 = $root;
 $63 = $min;
 $64 = ($62>>>0)<($63>>>0);
 if ($64) {
  $65 = $min;
  $root = $65;
 }
 $left = 1;
 $len = 1;
 while(1) {
  $66 = $len;
  $67 = ($66>>>0)<=(15);
  if (!($67)) {
   break;
  }
  $68 = $left;
  $69 = $68 << 1;
  $left = $69;
  $70 = $len;
  $71 = (($count) + ($70<<1)|0);
  $72 = HEAP16[$71>>1]|0;
  $73 = $72&65535;
  $74 = $left;
  $75 = (($74) - ($73))|0;
  $left = $75;
  $76 = $left;
  $77 = ($76|0)<(0);
  if ($77) {
   label = 30;
   break;
  }
  $78 = $len;
  $79 = (($78) + 1)|0;
  $len = $79;
 }
 if ((label|0) == 30) {
  $0 = -1;
  $346 = $0;
  STACKTOP = sp;return ($346|0);
 }
 $80 = $left;
 $81 = ($80|0)>(0);
 do {
  if ($81) {
   $82 = $1;
   $83 = ($82|0)==(0);
   if (!($83)) {
    $84 = $max;
    $85 = ($84|0)!=(1);
    if (!($85)) {
     break;
    }
   }
   $0 = -1;
   $346 = $0;
   STACKTOP = sp;return ($346|0);
  }
 } while(0);
 $86 = (($offs) + 2|0);
 HEAP16[$86>>1] = 0;
 $len = 1;
 while(1) {
  $87 = $len;
  $88 = ($87>>>0)<(15);
  if (!($88)) {
   break;
  }
  $89 = $len;
  $90 = (($offs) + ($89<<1)|0);
  $91 = HEAP16[$90>>1]|0;
  $92 = $91&65535;
  $93 = $len;
  $94 = (($count) + ($93<<1)|0);
  $95 = HEAP16[$94>>1]|0;
  $96 = $95&65535;
  $97 = (($92) + ($96))|0;
  $98 = $97&65535;
  $99 = $len;
  $100 = (($99) + 1)|0;
  $101 = (($offs) + ($100<<1)|0);
  HEAP16[$101>>1] = $98;
  $102 = $len;
  $103 = (($102) + 1)|0;
  $len = $103;
 }
 $sym = 0;
 while(1) {
  $104 = $sym;
  $105 = $3;
  $106 = ($104>>>0)<($105>>>0);
  if (!($106)) {
   break;
  }
  $107 = $sym;
  $108 = $2;
  $109 = (($108) + ($107<<1)|0);
  $110 = HEAP16[$109>>1]|0;
  $111 = $110&65535;
  $112 = ($111|0)!=(0);
  if ($112) {
   $113 = $sym;
   $114 = $113&65535;
   $115 = $sym;
   $116 = $2;
   $117 = (($116) + ($115<<1)|0);
   $118 = HEAP16[$117>>1]|0;
   $119 = $118&65535;
   $120 = (($offs) + ($119<<1)|0);
   $121 = HEAP16[$120>>1]|0;
   $122 = (($121) + 1)<<16>>16;
   HEAP16[$120>>1] = $122;
   $123 = $121&65535;
   $124 = $6;
   $125 = (($124) + ($123<<1)|0);
   HEAP16[$125>>1] = $114;
  }
  $126 = $sym;
  $127 = (($126) + 1)|0;
  $sym = $127;
 }
 $128 = $1;
 if ((($128|0) == 0)) {
  $129 = $6;
  $extra = $129;
  $base = $129;
  $end = 19;
 } else if ((($128|0) == 1)) {
  $base = 16;
  $130 = $base;
  $131 = (($130) + -514|0);
  $base = $131;
  $extra = 80;
  $132 = $extra;
  $133 = (($132) + -514|0);
  $extra = $133;
  $end = 256;
 } else {
  $base = 144;
  $extra = 208;
  $end = -1;
 }
 $huff = 0;
 $sym = 0;
 $134 = $min;
 $len = $134;
 $135 = $4;
 $136 = HEAP32[$135>>2]|0;
 $next = $136;
 $137 = $root;
 $curr = $137;
 $drop = 0;
 $low = -1;
 $138 = $root;
 $139 = 1 << $138;
 $used = $139;
 $140 = $used;
 $141 = (($140) - 1)|0;
 $mask = $141;
 $142 = $1;
 $143 = ($142|0)==(1);
 if ($143) {
  $144 = $used;
  $145 = ($144>>>0)>(852);
  if (!($145)) {
   label = 53;
  }
 } else {
  label = 53;
 }
 do {
  if ((label|0) == 53) {
   $146 = $1;
   $147 = ($146|0)==(2);
   if ($147) {
    $148 = $used;
    $149 = ($148>>>0)>(592);
    if ($149) {
     break;
    }
   }
   while(1) {
    $150 = $len;
    $151 = $drop;
    $152 = (($150) - ($151))|0;
    $153 = $152&255;
    $154 = (($here) + 1|0);
    HEAP8[$154] = $153;
    $155 = $sym;
    $156 = $6;
    $157 = (($156) + ($155<<1)|0);
    $158 = HEAP16[$157>>1]|0;
    $159 = $158&65535;
    $160 = $end;
    $161 = ($159|0)<($160|0);
    if ($161) {
     HEAP8[$here] = 0;
     $162 = $sym;
     $163 = $6;
     $164 = (($163) + ($162<<1)|0);
     $165 = HEAP16[$164>>1]|0;
     $166 = (($here) + 2|0);
     HEAP16[$166>>1] = $165;
    } else {
     $167 = $sym;
     $168 = $6;
     $169 = (($168) + ($167<<1)|0);
     $170 = HEAP16[$169>>1]|0;
     $171 = $170&65535;
     $172 = $end;
     $173 = ($171|0)>($172|0);
     if ($173) {
      $174 = $sym;
      $175 = $6;
      $176 = (($175) + ($174<<1)|0);
      $177 = HEAP16[$176>>1]|0;
      $178 = $177&65535;
      $179 = $extra;
      $180 = (($179) + ($178<<1)|0);
      $181 = HEAP16[$180>>1]|0;
      $182 = $181&255;
      HEAP8[$here] = $182;
      $183 = $sym;
      $184 = $6;
      $185 = (($184) + ($183<<1)|0);
      $186 = HEAP16[$185>>1]|0;
      $187 = $186&65535;
      $188 = $base;
      $189 = (($188) + ($187<<1)|0);
      $190 = HEAP16[$189>>1]|0;
      $191 = (($here) + 2|0);
      HEAP16[$191>>1] = $190;
     } else {
      HEAP8[$here] = 96;
      $192 = (($here) + 2|0);
      HEAP16[$192>>1] = 0;
     }
    }
    $193 = $len;
    $194 = $drop;
    $195 = (($193) - ($194))|0;
    $196 = 1 << $195;
    $incr = $196;
    $197 = $curr;
    $198 = 1 << $197;
    $fill = $198;
    $199 = $fill;
    $min = $199;
    while(1) {
     $200 = $incr;
     $201 = $fill;
     $202 = (($201) - ($200))|0;
     $fill = $202;
     $203 = $huff;
     $204 = $drop;
     $205 = $203 >>> $204;
     $206 = $fill;
     $207 = (($205) + ($206))|0;
     $208 = $next;
     $209 = (($208) + ($207<<2)|0);
     ;HEAP16[$209+0>>1]=HEAP16[$here+0>>1]|0;HEAP16[$209+2>>1]=HEAP16[$here+2>>1]|0;
     $210 = $fill;
     $211 = ($210|0)!=(0);
     if (!($211)) {
      break;
     }
    }
    $212 = $len;
    $213 = (($212) - 1)|0;
    $214 = 1 << $213;
    $incr = $214;
    while(1) {
     $215 = $huff;
     $216 = $incr;
     $217 = $215 & $216;
     $218 = ($217|0)!=(0);
     if (!($218)) {
      break;
     }
     $219 = $incr;
     $220 = $219 >>> 1;
     $incr = $220;
    }
    $221 = $incr;
    $222 = ($221|0)!=(0);
    if ($222) {
     $223 = $incr;
     $224 = (($223) - 1)|0;
     $225 = $huff;
     $226 = $225 & $224;
     $huff = $226;
     $227 = $incr;
     $228 = $huff;
     $229 = (($228) + ($227))|0;
     $huff = $229;
    } else {
     $huff = 0;
    }
    $230 = $sym;
    $231 = (($230) + 1)|0;
    $sym = $231;
    $232 = $len;
    $233 = (($count) + ($232<<1)|0);
    $234 = HEAP16[$233>>1]|0;
    $235 = (($234) + -1)<<16>>16;
    HEAP16[$233>>1] = $235;
    $236 = $235&65535;
    $237 = ($236|0)==(0);
    if ($237) {
     $238 = $len;
     $239 = $max;
     $240 = ($238|0)==($239|0);
     if ($240) {
      break;
     }
     $241 = $sym;
     $242 = $6;
     $243 = (($242) + ($241<<1)|0);
     $244 = HEAP16[$243>>1]|0;
     $245 = $244&65535;
     $246 = $2;
     $247 = (($246) + ($245<<1)|0);
     $248 = HEAP16[$247>>1]|0;
     $249 = $248&65535;
     $len = $249;
    }
    $250 = $len;
    $251 = $root;
    $252 = ($250>>>0)>($251>>>0);
    if ($252) {
     $253 = $huff;
     $254 = $mask;
     $255 = $253 & $254;
     $256 = $low;
     $257 = ($255|0)!=($256|0);
     if ($257) {
      $258 = $drop;
      $259 = ($258|0)==(0);
      if ($259) {
       $260 = $root;
       $drop = $260;
      }
      $261 = $min;
      $262 = $next;
      $263 = (($262) + ($261<<2)|0);
      $next = $263;
      $264 = $len;
      $265 = $drop;
      $266 = (($264) - ($265))|0;
      $curr = $266;
      $267 = $curr;
      $268 = 1 << $267;
      $left = $268;
      while(1) {
       $269 = $curr;
       $270 = $drop;
       $271 = (($269) + ($270))|0;
       $272 = $max;
       $273 = ($271>>>0)<($272>>>0);
       if (!($273)) {
        break;
       }
       $274 = $curr;
       $275 = $drop;
       $276 = (($274) + ($275))|0;
       $277 = (($count) + ($276<<1)|0);
       $278 = HEAP16[$277>>1]|0;
       $279 = $278&65535;
       $280 = $left;
       $281 = (($280) - ($279))|0;
       $left = $281;
       $282 = $left;
       $283 = ($282|0)<=(0);
       if ($283) {
        label = 83;
        break;
       }
       $284 = $curr;
       $285 = (($284) + 1)|0;
       $curr = $285;
       $286 = $left;
       $287 = $286 << 1;
       $left = $287;
      }
      if ((label|0) == 83) {
       label = 0;
      }
      $288 = $curr;
      $289 = 1 << $288;
      $290 = $used;
      $291 = (($290) + ($289))|0;
      $used = $291;
      $292 = $1;
      $293 = ($292|0)==(1);
      if ($293) {
       $294 = $used;
       $295 = ($294>>>0)>(852);
       if ($295) {
        label = 89;
        break;
       }
      }
      $296 = $1;
      $297 = ($296|0)==(2);
      if ($297) {
       $298 = $used;
       $299 = ($298>>>0)>(592);
       if ($299) {
        label = 89;
        break;
       }
      }
      $300 = $huff;
      $301 = $mask;
      $302 = $300 & $301;
      $low = $302;
      $303 = $curr;
      $304 = $303&255;
      $305 = $low;
      $306 = $4;
      $307 = HEAP32[$306>>2]|0;
      $308 = (($307) + ($305<<2)|0);
      HEAP8[$308] = $304;
      $309 = $root;
      $310 = $309&255;
      $311 = $low;
      $312 = $4;
      $313 = HEAP32[$312>>2]|0;
      $314 = (($313) + ($311<<2)|0);
      $315 = (($314) + 1|0);
      HEAP8[$315] = $310;
      $316 = $next;
      $317 = $4;
      $318 = HEAP32[$317>>2]|0;
      $319 = $316;
      $320 = $318;
      $321 = (($319) - ($320))|0;
      $322 = (($321|0) / 4)&-1;
      $323 = $322&65535;
      $324 = $low;
      $325 = $4;
      $326 = HEAP32[$325>>2]|0;
      $327 = (($326) + ($324<<2)|0);
      $328 = (($327) + 2|0);
      HEAP16[$328>>1] = $323;
     }
    }
   }
   if ((label|0) == 89) {
    $0 = 1;
    $346 = $0;
    STACKTOP = sp;return ($346|0);
   }
   $329 = $huff;
   $330 = ($329|0)!=(0);
   if ($330) {
    HEAP8[$here] = 64;
    $331 = $len;
    $332 = $drop;
    $333 = (($331) - ($332))|0;
    $334 = $333&255;
    $335 = (($here) + 1|0);
    HEAP8[$335] = $334;
    $336 = (($here) + 2|0);
    HEAP16[$336>>1] = 0;
    $337 = $huff;
    $338 = $next;
    $339 = (($338) + ($337<<2)|0);
    ;HEAP16[$339+0>>1]=HEAP16[$here+0>>1]|0;HEAP16[$339+2>>1]=HEAP16[$here+2>>1]|0;
   }
   $340 = $used;
   $341 = $4;
   $342 = HEAP32[$341>>2]|0;
   $343 = (($342) + ($340<<2)|0);
   HEAP32[$341>>2] = $343;
   $344 = $root;
   $345 = $5;
   HEAP32[$345>>2] = $344;
   $0 = 0;
   $346 = $0;
   STACKTOP = sp;return ($346|0);
  }
 } while(0);
 $0 = 1;
 $346 = $0;
 STACKTOP = sp;return ($346|0);
}
function _crc32($crc,$buf,$len) {
 $crc = $crc|0;
 $buf = $buf|0;
 $len = $len|0;
 var $0 = 0, $1 = 0, $10 = 0, $11 = 0, $12 = 0, $13 = 0, $14 = 0, $15 = 0, $16 = 0, $2 = 0, $3 = 0, $4 = 0, $5 = 0, $6 = 0, $7 = 0, $8 = 0, $9 = 0, $endian = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 32|0;
 $endian = sp + 12|0;
 $1 = $crc;
 $2 = $buf;
 $3 = $len;
 $4 = $2;
 $5 = ($4|0)==(0|0);
 do {
  if ($5) {
   $0 = 0;
  } else {
   HEAP32[$endian>>2] = 1;
   $6 = HEAP8[$endian]|0;
   $7 = ($6<<24>>24)!=(0);
   if ($7) {
    $8 = $1;
    $9 = $2;
    $10 = $3;
    $11 = (_crc32_little($8,$9,$10)|0);
    $0 = $11;
    break;
   } else {
    $12 = $1;
    $13 = $2;
    $14 = $3;
    $15 = (_crc32_big($12,$13,$14)|0);
    $0 = $15;
    break;
   }
  }
 } while(0);
 $16 = $0;
 STACKTOP = sp;return ($16|0);
}
function _crc32_little($crc,$buf,$len) {
 $crc = $crc|0;
 $buf = $buf|0;
 $len = $len|0;
 var $0 = 0, $1 = 0, $10 = 0, $100 = 0, $101 = 0, $102 = 0, $103 = 0, $104 = 0, $105 = 0, $106 = 0, $107 = 0, $108 = 0, $109 = 0, $11 = 0, $110 = 0, $111 = 0, $112 = 0, $113 = 0, $114 = 0, $115 = 0;
 var $116 = 0, $117 = 0, $118 = 0, $119 = 0, $12 = 0, $120 = 0, $121 = 0, $122 = 0, $123 = 0, $124 = 0, $125 = 0, $126 = 0, $127 = 0, $128 = 0, $129 = 0, $13 = 0, $130 = 0, $131 = 0, $132 = 0, $133 = 0;
 var $134 = 0, $135 = 0, $136 = 0, $137 = 0, $138 = 0, $139 = 0, $14 = 0, $140 = 0, $141 = 0, $142 = 0, $143 = 0, $144 = 0, $145 = 0, $146 = 0, $147 = 0, $148 = 0, $149 = 0, $15 = 0, $150 = 0, $151 = 0;
 var $152 = 0, $153 = 0, $154 = 0, $155 = 0, $156 = 0, $157 = 0, $158 = 0, $159 = 0, $16 = 0, $160 = 0, $161 = 0, $162 = 0, $163 = 0, $164 = 0, $165 = 0, $166 = 0, $167 = 0, $168 = 0, $169 = 0, $17 = 0;
 var $170 = 0, $171 = 0, $172 = 0, $173 = 0, $174 = 0, $175 = 0, $176 = 0, $177 = 0, $178 = 0, $179 = 0, $18 = 0, $180 = 0, $181 = 0, $182 = 0, $183 = 0, $184 = 0, $185 = 0, $186 = 0, $187 = 0, $188 = 0;
 var $189 = 0, $19 = 0, $190 = 0, $191 = 0, $192 = 0, $193 = 0, $194 = 0, $195 = 0, $196 = 0, $197 = 0, $198 = 0, $199 = 0, $2 = 0, $20 = 0, $200 = 0, $201 = 0, $202 = 0, $203 = 0, $204 = 0, $205 = 0;
 var $206 = 0, $207 = 0, $208 = 0, $209 = 0, $21 = 0, $210 = 0, $211 = 0, $212 = 0, $213 = 0, $214 = 0, $215 = 0, $216 = 0, $217 = 0, $218 = 0, $219 = 0, $22 = 0, $220 = 0, $221 = 0, $222 = 0, $223 = 0;
 var $224 = 0, $225 = 0, $226 = 0, $227 = 0, $228 = 0, $229 = 0, $23 = 0, $230 = 0, $231 = 0, $232 = 0, $233 = 0, $234 = 0, $235 = 0, $236 = 0, $237 = 0, $238 = 0, $239 = 0, $24 = 0, $240 = 0, $241 = 0;
 var $242 = 0, $243 = 0, $244 = 0, $245 = 0, $246 = 0, $247 = 0, $248 = 0, $249 = 0, $25 = 0, $250 = 0, $251 = 0, $252 = 0, $253 = 0, $254 = 0, $255 = 0, $256 = 0, $257 = 0, $258 = 0, $259 = 0, $26 = 0;
 var $260 = 0, $261 = 0, $262 = 0, $263 = 0, $264 = 0, $265 = 0, $266 = 0, $267 = 0, $268 = 0, $269 = 0, $27 = 0, $270 = 0, $271 = 0, $272 = 0, $273 = 0, $274 = 0, $275 = 0, $276 = 0, $277 = 0, $278 = 0;
 var $279 = 0, $28 = 0, $280 = 0, $281 = 0, $282 = 0, $283 = 0, $284 = 0, $285 = 0, $286 = 0, $287 = 0, $288 = 0, $289 = 0, $29 = 0, $290 = 0, $3 = 0, $30 = 0, $31 = 0, $32 = 0, $33 = 0, $34 = 0;
 var $35 = 0, $36 = 0, $37 = 0, $38 = 0, $39 = 0, $4 = 0, $40 = 0, $41 = 0, $42 = 0, $43 = 0, $44 = 0, $45 = 0, $46 = 0, $47 = 0, $48 = 0, $49 = 0, $5 = 0, $50 = 0, $51 = 0, $52 = 0;
 var $53 = 0, $54 = 0, $55 = 0, $56 = 0, $57 = 0, $58 = 0, $59 = 0, $6 = 0, $60 = 0, $61 = 0, $62 = 0, $63 = 0, $64 = 0, $65 = 0, $66 = 0, $67 = 0, $68 = 0, $69 = 0, $7 = 0, $70 = 0;
 var $71 = 0, $72 = 0, $73 = 0, $74 = 0, $75 = 0, $76 = 0, $77 = 0, $78 = 0, $79 = 0, $8 = 0, $80 = 0, $81 = 0, $82 = 0, $83 = 0, $84 = 0, $85 = 0, $86 = 0, $87 = 0, $88 = 0, $89 = 0;
 var $9 = 0, $90 = 0, $91 = 0, $92 = 0, $93 = 0, $94 = 0, $95 = 0, $96 = 0, $97 = 0, $98 = 0, $99 = 0, $buf4 = 0, $c = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 32|0;
 $0 = $crc;
 $1 = $buf;
 $2 = $len;
 $3 = $0;
 $c = $3;
 $4 = $c;
 $5 = $4 ^ -1;
 $c = $5;
 while(1) {
  $6 = $2;
  $7 = ($6|0)!=(0);
  if ($7) {
   $8 = $1;
   $9 = $8;
   $10 = $9 & 3;
   $11 = ($10|0)!=(0);
   $290 = $11;
  } else {
   $290 = 0;
  }
  if (!($290)) {
   break;
  }
  $12 = $c;
  $13 = $1;
  $14 = (($13) + 1|0);
  $1 = $14;
  $15 = HEAP8[$13]|0;
  $16 = $15&255;
  $17 = $12 ^ $16;
  $18 = $17 & 255;
  $19 = (272 + ($18<<2)|0);
  $20 = HEAP32[$19>>2]|0;
  $21 = $c;
  $22 = $21 >>> 8;
  $23 = $20 ^ $22;
  $c = $23;
  $24 = $2;
  $25 = (($24) + -1)|0;
  $2 = $25;
 }
 $26 = $1;
 $buf4 = $26;
 while(1) {
  $27 = $2;
  $28 = ($27>>>0)>=(32);
  if (!($28)) {
   break;
  }
  $29 = $buf4;
  $30 = (($29) + 4|0);
  $buf4 = $30;
  $31 = HEAP32[$29>>2]|0;
  $32 = $c;
  $33 = $32 ^ $31;
  $c = $33;
  $34 = $c;
  $35 = $34 & 255;
  $36 = (((272 + 3072|0)) + ($35<<2)|0);
  $37 = HEAP32[$36>>2]|0;
  $38 = $c;
  $39 = $38 >>> 8;
  $40 = $39 & 255;
  $41 = (((272 + 2048|0)) + ($40<<2)|0);
  $42 = HEAP32[$41>>2]|0;
  $43 = $37 ^ $42;
  $44 = $c;
  $45 = $44 >>> 16;
  $46 = $45 & 255;
  $47 = (((272 + 1024|0)) + ($46<<2)|0);
  $48 = HEAP32[$47>>2]|0;
  $49 = $43 ^ $48;
  $50 = $c;
  $51 = $50 >>> 24;
  $52 = (272 + ($51<<2)|0);
  $53 = HEAP32[$52>>2]|0;
  $54 = $49 ^ $53;
  $c = $54;
  $55 = $buf4;
  $56 = (($55) + 4|0);
  $buf4 = $56;
  $57 = HEAP32[$55>>2]|0;
  $58 = $c;
  $59 = $58 ^ $57;
  $c = $59;
  $60 = $c;
  $61 = $60 & 255;
  $62 = (((272 + 3072|0)) + ($61<<2)|0);
  $63 = HEAP32[$62>>2]|0;
  $64 = $c;
  $65 = $64 >>> 8;
  $66 = $65 & 255;
  $67 = (((272 + 2048|0)) + ($66<<2)|0);
  $68 = HEAP32[$67>>2]|0;
  $69 = $63 ^ $68;
  $70 = $c;
  $71 = $70 >>> 16;
  $72 = $71 & 255;
  $73 = (((272 + 1024|0)) + ($72<<2)|0);
  $74 = HEAP32[$73>>2]|0;
  $75 = $69 ^ $74;
  $76 = $c;
  $77 = $76 >>> 24;
  $78 = (272 + ($77<<2)|0);
  $79 = HEAP32[$78>>2]|0;
  $80 = $75 ^ $79;
  $c = $80;
  $81 = $buf4;
  $82 = (($81) + 4|0);
  $buf4 = $82;
  $83 = HEAP32[$81>>2]|0;
  $84 = $c;
  $85 = $84 ^ $83;
  $c = $85;
  $86 = $c;
  $87 = $86 & 255;
  $88 = (((272 + 3072|0)) + ($87<<2)|0);
  $89 = HEAP32[$88>>2]|0;
  $90 = $c;
  $91 = $90 >>> 8;
  $92 = $91 & 255;
  $93 = (((272 + 2048|0)) + ($92<<2)|0);
  $94 = HEAP32[$93>>2]|0;
  $95 = $89 ^ $94;
  $96 = $c;
  $97 = $96 >>> 16;
  $98 = $97 & 255;
  $99 = (((272 + 1024|0)) + ($98<<2)|0);
  $100 = HEAP32[$99>>2]|0;
  $101 = $95 ^ $100;
  $102 = $c;
  $103 = $102 >>> 24;
  $104 = (272 + ($103<<2)|0);
  $105 = HEAP32[$104>>2]|0;
  $106 = $101 ^ $105;
  $c = $106;
  $107 = $buf4;
  $108 = (($107) + 4|0);
  $buf4 = $108;
  $109 = HEAP32[$107>>2]|0;
  $110 = $c;
  $111 = $110 ^ $109;
  $c = $111;
  $112 = $c;
  $113 = $112 & 255;
  $114 = (((272 + 3072|0)) + ($113<<2)|0);
  $115 = HEAP32[$114>>2]|0;
  $116 = $c;
  $117 = $116 >>> 8;
  $118 = $117 & 255;
  $119 = (((272 + 2048|0)) + ($118<<2)|0);
  $120 = HEAP32[$119>>2]|0;
  $121 = $115 ^ $120;
  $122 = $c;
  $123 = $122 >>> 16;
  $124 = $123 & 255;
  $125 = (((272 + 1024|0)) + ($124<<2)|0);
  $126 = HEAP32[$125>>2]|0;
  $127 = $121 ^ $126;
  $128 = $c;
  $129 = $128 >>> 24;
  $130 = (272 + ($129<<2)|0);
  $131 = HEAP32[$130>>2]|0;
  $132 = $127 ^ $131;
  $c = $132;
  $133 = $buf4;
  $134 = (($133) + 4|0);
  $buf4 = $134;
  $135 = HEAP32[$133>>2]|0;
  $136 = $c;
  $137 = $136 ^ $135;
  $c = $137;
  $138 = $c;
  $139 = $138 & 255;
  $140 = (((272 + 3072|0)) + ($139<<2)|0);
  $141 = HEAP32[$140>>2]|0;
  $142 = $c;
  $143 = $142 >>> 8;
  $144 = $143 & 255;
  $145 = (((272 + 2048|0)) + ($144<<2)|0);
  $146 = HEAP32[$145>>2]|0;
  $147 = $141 ^ $146;
  $148 = $c;
  $149 = $148 >>> 16;
  $150 = $149 & 255;
  $151 = (((272 + 1024|0)) + ($150<<2)|0);
  $152 = HEAP32[$151>>2]|0;
  $153 = $147 ^ $152;
  $154 = $c;
  $155 = $154 >>> 24;
  $156 = (272 + ($155<<2)|0);
  $157 = HEAP32[$156>>2]|0;
  $158 = $153 ^ $157;
  $c = $158;
  $159 = $buf4;
  $160 = (($159) + 4|0);
  $buf4 = $160;
  $161 = HEAP32[$159>>2]|0;
  $162 = $c;
  $163 = $162 ^ $161;
  $c = $163;
  $164 = $c;
  $165 = $164 & 255;
  $166 = (((272 + 3072|0)) + ($165<<2)|0);
  $167 = HEAP32[$166>>2]|0;
  $168 = $c;
  $169 = $168 >>> 8;
  $170 = $169 & 255;
  $171 = (((272 + 2048|0)) + ($170<<2)|0);
  $172 = HEAP32[$171>>2]|0;
  $173 = $167 ^ $172;
  $174 = $c;
  $175 = $174 >>> 16;
  $176 = $175 & 255;
  $177 = (((272 + 1024|0)) + ($176<<2)|0);
  $178 = HEAP32[$177>>2]|0;
  $179 = $173 ^ $178;
  $180 = $c;
  $181 = $180 >>> 24;
  $182 = (272 + ($181<<2)|0);
  $183 = HEAP32[$182>>2]|0;
  $184 = $179 ^ $183;
  $c = $184;
  $185 = $buf4;
  $186 = (($185) + 4|0);
  $buf4 = $186;
  $187 = HEAP32[$185>>2]|0;
  $188 = $c;
  $189 = $188 ^ $187;
  $c = $189;
  $190 = $c;
  $191 = $190 & 255;
  $192 = (((272 + 3072|0)) + ($191<<2)|0);
  $193 = HEAP32[$192>>2]|0;
  $194 = $c;
  $195 = $194 >>> 8;
  $196 = $195 & 255;
  $197 = (((272 + 2048|0)) + ($196<<2)|0);
  $198 = HEAP32[$197>>2]|0;
  $199 = $193 ^ $198;
  $200 = $c;
  $201 = $200 >>> 16;
  $202 = $201 & 255;
  $203 = (((272 + 1024|0)) + ($202<<2)|0);
  $204 = HEAP32[$203>>2]|0;
  $205 = $199 ^ $204;
  $206 = $c;
  $207 = $206 >>> 24;
  $208 = (272 + ($207<<2)|0);
  $209 = HEAP32[$208>>2]|0;
  $210 = $205 ^ $209;
  $c = $210;
  $211 = $buf4;
  $212 = (($211) + 4|0);
  $buf4 = $212;
  $213 = HEAP32[$211>>2]|0;
  $214 = $c;
  $215 = $214 ^ $213;
  $c = $215;
  $216 = $c;
  $217 = $216 & 255;
  $218 = (((272 + 3072|0)) + ($217<<2)|0);
  $219 = HEAP32[$218>>2]|0;
  $220 = $c;
  $221 = $220 >>> 8;
  $222 = $221 & 255;
  $223 = (((272 + 2048|0)) + ($222<<2)|0);
  $224 = HEAP32[$223>>2]|0;
  $225 = $219 ^ $224;
  $226 = $c;
  $227 = $226 >>> 16;
  $228 = $227 & 255;
  $229 = (((272 + 1024|0)) + ($228<<2)|0);
  $230 = HEAP32[$229>>2]|0;
  $231 = $225 ^ $230;
  $232 = $c;
  $233 = $232 >>> 24;
  $234 = (272 + ($233<<2)|0);
  $235 = HEAP32[$234>>2]|0;
  $236 = $231 ^ $235;
  $c = $236;
  $237 = $2;
  $238 = (($237) - 32)|0;
  $2 = $238;
 }
 while(1) {
  $239 = $2;
  $240 = ($239>>>0)>=(4);
  if (!($240)) {
   break;
  }
  $241 = $buf4;
  $242 = (($241) + 4|0);
  $buf4 = $242;
  $243 = HEAP32[$241>>2]|0;
  $244 = $c;
  $245 = $244 ^ $243;
  $c = $245;
  $246 = $c;
  $247 = $246 & 255;
  $248 = (((272 + 3072|0)) + ($247<<2)|0);
  $249 = HEAP32[$248>>2]|0;
  $250 = $c;
  $251 = $250 >>> 8;
  $252 = $251 & 255;
  $253 = (((272 + 2048|0)) + ($252<<2)|0);
  $254 = HEAP32[$253>>2]|0;
  $255 = $249 ^ $254;
  $256 = $c;
  $257 = $256 >>> 16;
  $258 = $257 & 255;
  $259 = (((272 + 1024|0)) + ($258<<2)|0);
  $260 = HEAP32[$259>>2]|0;
  $261 = $255 ^ $260;
  $262 = $c;
  $263 = $262 >>> 24;
  $264 = (272 + ($263<<2)|0);
  $265 = HEAP32[$264>>2]|0;
  $266 = $261 ^ $265;
  $c = $266;
  $267 = $2;
  $268 = (($267) - 4)|0;
  $2 = $268;
 }
 $269 = $buf4;
 $1 = $269;
 $270 = $2;
 $271 = ($270|0)!=(0);
 if (!($271)) {
  $287 = $c;
  $288 = $287 ^ -1;
  $c = $288;
  $289 = $c;
  STACKTOP = sp;return ($289|0);
 }
 while(1) {
  $272 = $c;
  $273 = $1;
  $274 = (($273) + 1|0);
  $1 = $274;
  $275 = HEAP8[$273]|0;
  $276 = $275&255;
  $277 = $272 ^ $276;
  $278 = $277 & 255;
  $279 = (272 + ($278<<2)|0);
  $280 = HEAP32[$279>>2]|0;
  $281 = $c;
  $282 = $281 >>> 8;
  $283 = $280 ^ $282;
  $c = $283;
  $284 = $2;
  $285 = (($284) + -1)|0;
  $2 = $285;
  $286 = ($285|0)!=(0);
  if (!($286)) {
   break;
  }
 }
 $287 = $c;
 $288 = $287 ^ -1;
 $c = $288;
 $289 = $c;
 STACKTOP = sp;return ($289|0);
}
function _crc32_big($crc,$buf,$len) {
 $crc = $crc|0;
 $buf = $buf|0;
 $len = $len|0;
 var $0 = 0, $1 = 0, $10 = 0, $100 = 0, $101 = 0, $102 = 0, $103 = 0, $104 = 0, $105 = 0, $106 = 0, $107 = 0, $108 = 0, $109 = 0, $11 = 0, $110 = 0, $111 = 0, $112 = 0, $113 = 0, $114 = 0, $115 = 0;
 var $116 = 0, $117 = 0, $118 = 0, $119 = 0, $12 = 0, $120 = 0, $121 = 0, $122 = 0, $123 = 0, $124 = 0, $125 = 0, $126 = 0, $127 = 0, $128 = 0, $129 = 0, $13 = 0, $130 = 0, $131 = 0, $132 = 0, $133 = 0;
 var $134 = 0, $135 = 0, $136 = 0, $137 = 0, $138 = 0, $139 = 0, $14 = 0, $140 = 0, $141 = 0, $142 = 0, $143 = 0, $144 = 0, $145 = 0, $146 = 0, $147 = 0, $148 = 0, $149 = 0, $15 = 0, $150 = 0, $151 = 0;
 var $152 = 0, $153 = 0, $154 = 0, $155 = 0, $156 = 0, $157 = 0, $158 = 0, $159 = 0, $16 = 0, $160 = 0, $161 = 0, $162 = 0, $163 = 0, $164 = 0, $165 = 0, $166 = 0, $167 = 0, $168 = 0, $169 = 0, $17 = 0;
 var $170 = 0, $171 = 0, $172 = 0, $173 = 0, $174 = 0, $175 = 0, $176 = 0, $177 = 0, $178 = 0, $179 = 0, $18 = 0, $180 = 0, $181 = 0, $182 = 0, $183 = 0, $184 = 0, $185 = 0, $186 = 0, $187 = 0, $188 = 0;
 var $189 = 0, $19 = 0, $190 = 0, $191 = 0, $192 = 0, $193 = 0, $194 = 0, $195 = 0, $196 = 0, $197 = 0, $198 = 0, $199 = 0, $2 = 0, $20 = 0, $200 = 0, $201 = 0, $202 = 0, $203 = 0, $204 = 0, $205 = 0;
 var $206 = 0, $207 = 0, $208 = 0, $209 = 0, $21 = 0, $210 = 0, $211 = 0, $212 = 0, $213 = 0, $214 = 0, $215 = 0, $216 = 0, $217 = 0, $218 = 0, $219 = 0, $22 = 0, $220 = 0, $221 = 0, $222 = 0, $223 = 0;
 var $224 = 0, $225 = 0, $226 = 0, $227 = 0, $228 = 0, $229 = 0, $23 = 0, $230 = 0, $231 = 0, $232 = 0, $233 = 0, $234 = 0, $235 = 0, $236 = 0, $237 = 0, $238 = 0, $239 = 0, $24 = 0, $240 = 0, $241 = 0;
 var $242 = 0, $243 = 0, $244 = 0, $245 = 0, $246 = 0, $247 = 0, $248 = 0, $249 = 0, $25 = 0, $250 = 0, $251 = 0, $252 = 0, $253 = 0, $254 = 0, $255 = 0, $256 = 0, $257 = 0, $258 = 0, $259 = 0, $26 = 0;
 var $260 = 0, $261 = 0, $262 = 0, $263 = 0, $264 = 0, $265 = 0, $266 = 0, $267 = 0, $268 = 0, $269 = 0, $27 = 0, $270 = 0, $271 = 0, $272 = 0, $273 = 0, $274 = 0, $275 = 0, $276 = 0, $277 = 0, $278 = 0;
 var $279 = 0, $28 = 0, $280 = 0, $281 = 0, $282 = 0, $283 = 0, $284 = 0, $285 = 0, $286 = 0, $287 = 0, $288 = 0, $289 = 0, $29 = 0, $290 = 0, $291 = 0, $292 = 0, $293 = 0, $294 = 0, $295 = 0, $296 = 0;
 var $297 = 0, $298 = 0, $299 = 0, $3 = 0, $30 = 0, $300 = 0, $301 = 0, $302 = 0, $303 = 0, $304 = 0, $305 = 0, $306 = 0, $307 = 0, $308 = 0, $309 = 0, $31 = 0, $310 = 0, $311 = 0, $312 = 0, $313 = 0;
 var $314 = 0, $315 = 0, $316 = 0, $317 = 0, $318 = 0, $319 = 0, $32 = 0, $320 = 0, $321 = 0, $322 = 0, $33 = 0, $34 = 0, $35 = 0, $36 = 0, $37 = 0, $38 = 0, $39 = 0, $4 = 0, $40 = 0, $41 = 0;
 var $42 = 0, $43 = 0, $44 = 0, $45 = 0, $46 = 0, $47 = 0, $48 = 0, $49 = 0, $5 = 0, $50 = 0, $51 = 0, $52 = 0, $53 = 0, $54 = 0, $55 = 0, $56 = 0, $57 = 0, $58 = 0, $59 = 0, $6 = 0;
 var $60 = 0, $61 = 0, $62 = 0, $63 = 0, $64 = 0, $65 = 0, $66 = 0, $67 = 0, $68 = 0, $69 = 0, $7 = 0, $70 = 0, $71 = 0, $72 = 0, $73 = 0, $74 = 0, $75 = 0, $76 = 0, $77 = 0, $78 = 0;
 var $79 = 0, $8 = 0, $80 = 0, $81 = 0, $82 = 0, $83 = 0, $84 = 0, $85 = 0, $86 = 0, $87 = 0, $88 = 0, $89 = 0, $9 = 0, $90 = 0, $91 = 0, $92 = 0, $93 = 0, $94 = 0, $95 = 0, $96 = 0;
 var $97 = 0, $98 = 0, $99 = 0, $buf4 = 0, $c = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 32|0;
 $0 = $crc;
 $1 = $buf;
 $2 = $len;
 $3 = $0;
 $4 = $3 >>> 24;
 $5 = $4 & 255;
 $6 = $0;
 $7 = $6 >>> 8;
 $8 = $7 & 65280;
 $9 = (($5) + ($8))|0;
 $10 = $0;
 $11 = $10 & 65280;
 $12 = $11 << 8;
 $13 = (($9) + ($12))|0;
 $14 = $0;
 $15 = $14 & 255;
 $16 = $15 << 24;
 $17 = (($13) + ($16))|0;
 $c = $17;
 $18 = $c;
 $19 = $18 ^ -1;
 $c = $19;
 while(1) {
  $20 = $2;
  $21 = ($20|0)!=(0);
  if ($21) {
   $22 = $1;
   $23 = $22;
   $24 = $23 & 3;
   $25 = ($24|0)!=(0);
   $322 = $25;
  } else {
   $322 = 0;
  }
  if (!($322)) {
   break;
  }
  $26 = $c;
  $27 = $26 >>> 24;
  $28 = $1;
  $29 = (($28) + 1|0);
  $1 = $29;
  $30 = HEAP8[$28]|0;
  $31 = $30&255;
  $32 = $27 ^ $31;
  $33 = (((272 + 4096|0)) + ($32<<2)|0);
  $34 = HEAP32[$33>>2]|0;
  $35 = $c;
  $36 = $35 << 8;
  $37 = $34 ^ $36;
  $c = $37;
  $38 = $2;
  $39 = (($38) + -1)|0;
  $2 = $39;
 }
 $40 = $1;
 $buf4 = $40;
 $41 = $buf4;
 $42 = (($41) + -4|0);
 $buf4 = $42;
 while(1) {
  $43 = $2;
  $44 = ($43>>>0)>=(32);
  if (!($44)) {
   break;
  }
  $45 = $buf4;
  $46 = (($45) + 4|0);
  $buf4 = $46;
  $47 = HEAP32[$46>>2]|0;
  $48 = $c;
  $49 = $48 ^ $47;
  $c = $49;
  $50 = $c;
  $51 = $50 & 255;
  $52 = (((272 + 4096|0)) + ($51<<2)|0);
  $53 = HEAP32[$52>>2]|0;
  $54 = $c;
  $55 = $54 >>> 8;
  $56 = $55 & 255;
  $57 = (((272 + 5120|0)) + ($56<<2)|0);
  $58 = HEAP32[$57>>2]|0;
  $59 = $53 ^ $58;
  $60 = $c;
  $61 = $60 >>> 16;
  $62 = $61 & 255;
  $63 = (((272 + 6144|0)) + ($62<<2)|0);
  $64 = HEAP32[$63>>2]|0;
  $65 = $59 ^ $64;
  $66 = $c;
  $67 = $66 >>> 24;
  $68 = (((272 + 7168|0)) + ($67<<2)|0);
  $69 = HEAP32[$68>>2]|0;
  $70 = $65 ^ $69;
  $c = $70;
  $71 = $buf4;
  $72 = (($71) + 4|0);
  $buf4 = $72;
  $73 = HEAP32[$72>>2]|0;
  $74 = $c;
  $75 = $74 ^ $73;
  $c = $75;
  $76 = $c;
  $77 = $76 & 255;
  $78 = (((272 + 4096|0)) + ($77<<2)|0);
  $79 = HEAP32[$78>>2]|0;
  $80 = $c;
  $81 = $80 >>> 8;
  $82 = $81 & 255;
  $83 = (((272 + 5120|0)) + ($82<<2)|0);
  $84 = HEAP32[$83>>2]|0;
  $85 = $79 ^ $84;
  $86 = $c;
  $87 = $86 >>> 16;
  $88 = $87 & 255;
  $89 = (((272 + 6144|0)) + ($88<<2)|0);
  $90 = HEAP32[$89>>2]|0;
  $91 = $85 ^ $90;
  $92 = $c;
  $93 = $92 >>> 24;
  $94 = (((272 + 7168|0)) + ($93<<2)|0);
  $95 = HEAP32[$94>>2]|0;
  $96 = $91 ^ $95;
  $c = $96;
  $97 = $buf4;
  $98 = (($97) + 4|0);
  $buf4 = $98;
  $99 = HEAP32[$98>>2]|0;
  $100 = $c;
  $101 = $100 ^ $99;
  $c = $101;
  $102 = $c;
  $103 = $102 & 255;
  $104 = (((272 + 4096|0)) + ($103<<2)|0);
  $105 = HEAP32[$104>>2]|0;
  $106 = $c;
  $107 = $106 >>> 8;
  $108 = $107 & 255;
  $109 = (((272 + 5120|0)) + ($108<<2)|0);
  $110 = HEAP32[$109>>2]|0;
  $111 = $105 ^ $110;
  $112 = $c;
  $113 = $112 >>> 16;
  $114 = $113 & 255;
  $115 = (((272 + 6144|0)) + ($114<<2)|0);
  $116 = HEAP32[$115>>2]|0;
  $117 = $111 ^ $116;
  $118 = $c;
  $119 = $118 >>> 24;
  $120 = (((272 + 7168|0)) + ($119<<2)|0);
  $121 = HEAP32[$120>>2]|0;
  $122 = $117 ^ $121;
  $c = $122;
  $123 = $buf4;
  $124 = (($123) + 4|0);
  $buf4 = $124;
  $125 = HEAP32[$124>>2]|0;
  $126 = $c;
  $127 = $126 ^ $125;
  $c = $127;
  $128 = $c;
  $129 = $128 & 255;
  $130 = (((272 + 4096|0)) + ($129<<2)|0);
  $131 = HEAP32[$130>>2]|0;
  $132 = $c;
  $133 = $132 >>> 8;
  $134 = $133 & 255;
  $135 = (((272 + 5120|0)) + ($134<<2)|0);
  $136 = HEAP32[$135>>2]|0;
  $137 = $131 ^ $136;
  $138 = $c;
  $139 = $138 >>> 16;
  $140 = $139 & 255;
  $141 = (((272 + 6144|0)) + ($140<<2)|0);
  $142 = HEAP32[$141>>2]|0;
  $143 = $137 ^ $142;
  $144 = $c;
  $145 = $144 >>> 24;
  $146 = (((272 + 7168|0)) + ($145<<2)|0);
  $147 = HEAP32[$146>>2]|0;
  $148 = $143 ^ $147;
  $c = $148;
  $149 = $buf4;
  $150 = (($149) + 4|0);
  $buf4 = $150;
  $151 = HEAP32[$150>>2]|0;
  $152 = $c;
  $153 = $152 ^ $151;
  $c = $153;
  $154 = $c;
  $155 = $154 & 255;
  $156 = (((272 + 4096|0)) + ($155<<2)|0);
  $157 = HEAP32[$156>>2]|0;
  $158 = $c;
  $159 = $158 >>> 8;
  $160 = $159 & 255;
  $161 = (((272 + 5120|0)) + ($160<<2)|0);
  $162 = HEAP32[$161>>2]|0;
  $163 = $157 ^ $162;
  $164 = $c;
  $165 = $164 >>> 16;
  $166 = $165 & 255;
  $167 = (((272 + 6144|0)) + ($166<<2)|0);
  $168 = HEAP32[$167>>2]|0;
  $169 = $163 ^ $168;
  $170 = $c;
  $171 = $170 >>> 24;
  $172 = (((272 + 7168|0)) + ($171<<2)|0);
  $173 = HEAP32[$172>>2]|0;
  $174 = $169 ^ $173;
  $c = $174;
  $175 = $buf4;
  $176 = (($175) + 4|0);
  $buf4 = $176;
  $177 = HEAP32[$176>>2]|0;
  $178 = $c;
  $179 = $178 ^ $177;
  $c = $179;
  $180 = $c;
  $181 = $180 & 255;
  $182 = (((272 + 4096|0)) + ($181<<2)|0);
  $183 = HEAP32[$182>>2]|0;
  $184 = $c;
  $185 = $184 >>> 8;
  $186 = $185 & 255;
  $187 = (((272 + 5120|0)) + ($186<<2)|0);
  $188 = HEAP32[$187>>2]|0;
  $189 = $183 ^ $188;
  $190 = $c;
  $191 = $190 >>> 16;
  $192 = $191 & 255;
  $193 = (((272 + 6144|0)) + ($192<<2)|0);
  $194 = HEAP32[$193>>2]|0;
  $195 = $189 ^ $194;
  $196 = $c;
  $197 = $196 >>> 24;
  $198 = (((272 + 7168|0)) + ($197<<2)|0);
  $199 = HEAP32[$198>>2]|0;
  $200 = $195 ^ $199;
  $c = $200;
  $201 = $buf4;
  $202 = (($201) + 4|0);
  $buf4 = $202;
  $203 = HEAP32[$202>>2]|0;
  $204 = $c;
  $205 = $204 ^ $203;
  $c = $205;
  $206 = $c;
  $207 = $206 & 255;
  $208 = (((272 + 4096|0)) + ($207<<2)|0);
  $209 = HEAP32[$208>>2]|0;
  $210 = $c;
  $211 = $210 >>> 8;
  $212 = $211 & 255;
  $213 = (((272 + 5120|0)) + ($212<<2)|0);
  $214 = HEAP32[$213>>2]|0;
  $215 = $209 ^ $214;
  $216 = $c;
  $217 = $216 >>> 16;
  $218 = $217 & 255;
  $219 = (((272 + 6144|0)) + ($218<<2)|0);
  $220 = HEAP32[$219>>2]|0;
  $221 = $215 ^ $220;
  $222 = $c;
  $223 = $222 >>> 24;
  $224 = (((272 + 7168|0)) + ($223<<2)|0);
  $225 = HEAP32[$224>>2]|0;
  $226 = $221 ^ $225;
  $c = $226;
  $227 = $buf4;
  $228 = (($227) + 4|0);
  $buf4 = $228;
  $229 = HEAP32[$228>>2]|0;
  $230 = $c;
  $231 = $230 ^ $229;
  $c = $231;
  $232 = $c;
  $233 = $232 & 255;
  $234 = (((272 + 4096|0)) + ($233<<2)|0);
  $235 = HEAP32[$234>>2]|0;
  $236 = $c;
  $237 = $236 >>> 8;
  $238 = $237 & 255;
  $239 = (((272 + 5120|0)) + ($238<<2)|0);
  $240 = HEAP32[$239>>2]|0;
  $241 = $235 ^ $240;
  $242 = $c;
  $243 = $242 >>> 16;
  $244 = $243 & 255;
  $245 = (((272 + 6144|0)) + ($244<<2)|0);
  $246 = HEAP32[$245>>2]|0;
  $247 = $241 ^ $246;
  $248 = $c;
  $249 = $248 >>> 24;
  $250 = (((272 + 7168|0)) + ($249<<2)|0);
  $251 = HEAP32[$250>>2]|0;
  $252 = $247 ^ $251;
  $c = $252;
  $253 = $2;
  $254 = (($253) - 32)|0;
  $2 = $254;
 }
 while(1) {
  $255 = $2;
  $256 = ($255>>>0)>=(4);
  if (!($256)) {
   break;
  }
  $257 = $buf4;
  $258 = (($257) + 4|0);
  $buf4 = $258;
  $259 = HEAP32[$258>>2]|0;
  $260 = $c;
  $261 = $260 ^ $259;
  $c = $261;
  $262 = $c;
  $263 = $262 & 255;
  $264 = (((272 + 4096|0)) + ($263<<2)|0);
  $265 = HEAP32[$264>>2]|0;
  $266 = $c;
  $267 = $266 >>> 8;
  $268 = $267 & 255;
  $269 = (((272 + 5120|0)) + ($268<<2)|0);
  $270 = HEAP32[$269>>2]|0;
  $271 = $265 ^ $270;
  $272 = $c;
  $273 = $272 >>> 16;
  $274 = $273 & 255;
  $275 = (((272 + 6144|0)) + ($274<<2)|0);
  $276 = HEAP32[$275>>2]|0;
  $277 = $271 ^ $276;
  $278 = $c;
  $279 = $278 >>> 24;
  $280 = (((272 + 7168|0)) + ($279<<2)|0);
  $281 = HEAP32[$280>>2]|0;
  $282 = $277 ^ $281;
  $c = $282;
  $283 = $2;
  $284 = (($283) - 4)|0;
  $2 = $284;
 }
 $285 = $buf4;
 $286 = (($285) + 4|0);
 $buf4 = $286;
 $287 = $buf4;
 $1 = $287;
 $288 = $2;
 $289 = ($288|0)!=(0);
 if (!($289)) {
  $305 = $c;
  $306 = $305 ^ -1;
  $c = $306;
  $307 = $c;
  $308 = $307 >>> 24;
  $309 = $308 & 255;
  $310 = $c;
  $311 = $310 >>> 8;
  $312 = $311 & 65280;
  $313 = (($309) + ($312))|0;
  $314 = $c;
  $315 = $314 & 65280;
  $316 = $315 << 8;
  $317 = (($313) + ($316))|0;
  $318 = $c;
  $319 = $318 & 255;
  $320 = $319 << 24;
  $321 = (($317) + ($320))|0;
  STACKTOP = sp;return ($321|0);
 }
 while(1) {
  $290 = $c;
  $291 = $290 >>> 24;
  $292 = $1;
  $293 = (($292) + 1|0);
  $1 = $293;
  $294 = HEAP8[$292]|0;
  $295 = $294&255;
  $296 = $291 ^ $295;
  $297 = (((272 + 4096|0)) + ($296<<2)|0);
  $298 = HEAP32[$297>>2]|0;
  $299 = $c;
  $300 = $299 << 8;
  $301 = $298 ^ $300;
  $c = $301;
  $302 = $2;
  $303 = (($302) + -1)|0;
  $2 = $303;
  $304 = ($303|0)!=(0);
  if (!($304)) {
   break;
  }
 }
 $305 = $c;
 $306 = $305 ^ -1;
 $c = $306;
 $307 = $c;
 $308 = $307 >>> 24;
 $309 = $308 & 255;
 $310 = $c;
 $311 = $310 >>> 8;
 $312 = $311 & 65280;
 $313 = (($309) + ($312))|0;
 $314 = $c;
 $315 = $314 & 65280;
 $316 = $315 << 8;
 $317 = (($313) + ($316))|0;
 $318 = $c;
 $319 = $318 & 255;
 $320 = $319 << 24;
 $321 = (($317) + ($320))|0;
 STACKTOP = sp;return ($321|0);
}
function _adler32($adler,$buf,$len) {
 $adler = $adler|0;
 $buf = $buf|0;
 $len = $len|0;
 var $0 = 0, $1 = 0, $10 = 0, $100 = 0, $101 = 0, $102 = 0, $103 = 0, $104 = 0, $105 = 0, $106 = 0, $107 = 0, $108 = 0, $109 = 0, $11 = 0, $110 = 0, $111 = 0, $112 = 0, $113 = 0, $114 = 0, $115 = 0;
 var $116 = 0, $117 = 0, $118 = 0, $119 = 0, $12 = 0, $120 = 0, $121 = 0, $122 = 0, $123 = 0, $124 = 0, $125 = 0, $126 = 0, $127 = 0, $128 = 0, $129 = 0, $13 = 0, $130 = 0, $131 = 0, $132 = 0, $133 = 0;
 var $134 = 0, $135 = 0, $136 = 0, $137 = 0, $138 = 0, $139 = 0, $14 = 0, $140 = 0, $141 = 0, $142 = 0, $143 = 0, $144 = 0, $145 = 0, $146 = 0, $147 = 0, $148 = 0, $149 = 0, $15 = 0, $150 = 0, $151 = 0;
 var $152 = 0, $153 = 0, $154 = 0, $155 = 0, $156 = 0, $157 = 0, $158 = 0, $159 = 0, $16 = 0, $160 = 0, $161 = 0, $162 = 0, $163 = 0, $164 = 0, $165 = 0, $166 = 0, $167 = 0, $168 = 0, $169 = 0, $17 = 0;
 var $170 = 0, $171 = 0, $172 = 0, $173 = 0, $174 = 0, $175 = 0, $176 = 0, $177 = 0, $178 = 0, $179 = 0, $18 = 0, $180 = 0, $181 = 0, $182 = 0, $183 = 0, $184 = 0, $185 = 0, $186 = 0, $187 = 0, $188 = 0;
 var $189 = 0, $19 = 0, $190 = 0, $191 = 0, $192 = 0, $193 = 0, $194 = 0, $195 = 0, $196 = 0, $197 = 0, $198 = 0, $199 = 0, $2 = 0, $20 = 0, $200 = 0, $201 = 0, $202 = 0, $203 = 0, $204 = 0, $205 = 0;
 var $206 = 0, $207 = 0, $208 = 0, $209 = 0, $21 = 0, $210 = 0, $211 = 0, $212 = 0, $213 = 0, $214 = 0, $215 = 0, $216 = 0, $217 = 0, $218 = 0, $219 = 0, $22 = 0, $220 = 0, $221 = 0, $222 = 0, $223 = 0;
 var $224 = 0, $225 = 0, $226 = 0, $227 = 0, $228 = 0, $229 = 0, $23 = 0, $230 = 0, $231 = 0, $232 = 0, $233 = 0, $234 = 0, $235 = 0, $236 = 0, $237 = 0, $238 = 0, $239 = 0, $24 = 0, $240 = 0, $241 = 0;
 var $242 = 0, $243 = 0, $244 = 0, $245 = 0, $246 = 0, $247 = 0, $248 = 0, $249 = 0, $25 = 0, $250 = 0, $251 = 0, $252 = 0, $253 = 0, $254 = 0, $255 = 0, $256 = 0, $257 = 0, $258 = 0, $259 = 0, $26 = 0;
 var $260 = 0, $261 = 0, $262 = 0, $263 = 0, $264 = 0, $265 = 0, $266 = 0, $267 = 0, $268 = 0, $269 = 0, $27 = 0, $270 = 0, $271 = 0, $272 = 0, $273 = 0, $274 = 0, $275 = 0, $276 = 0, $277 = 0, $278 = 0;
 var $279 = 0, $28 = 0, $280 = 0, $281 = 0, $282 = 0, $283 = 0, $284 = 0, $285 = 0, $286 = 0, $287 = 0, $288 = 0, $289 = 0, $29 = 0, $290 = 0, $291 = 0, $292 = 0, $293 = 0, $294 = 0, $295 = 0, $296 = 0;
 var $297 = 0, $298 = 0, $299 = 0, $3 = 0, $30 = 0, $300 = 0, $301 = 0, $302 = 0, $303 = 0, $304 = 0, $305 = 0, $306 = 0, $307 = 0, $308 = 0, $309 = 0, $31 = 0, $310 = 0, $311 = 0, $312 = 0, $313 = 0;
 var $314 = 0, $315 = 0, $316 = 0, $317 = 0, $318 = 0, $319 = 0, $32 = 0, $320 = 0, $321 = 0, $322 = 0, $323 = 0, $324 = 0, $325 = 0, $326 = 0, $327 = 0, $328 = 0, $329 = 0, $33 = 0, $330 = 0, $331 = 0;
 var $332 = 0, $333 = 0, $334 = 0, $335 = 0, $336 = 0, $337 = 0, $338 = 0, $339 = 0, $34 = 0, $340 = 0, $341 = 0, $342 = 0, $343 = 0, $344 = 0, $345 = 0, $346 = 0, $347 = 0, $348 = 0, $349 = 0, $35 = 0;
 var $350 = 0, $351 = 0, $352 = 0, $353 = 0, $354 = 0, $355 = 0, $356 = 0, $357 = 0, $358 = 0, $359 = 0, $36 = 0, $360 = 0, $361 = 0, $362 = 0, $363 = 0, $364 = 0, $365 = 0, $366 = 0, $367 = 0, $368 = 0;
 var $369 = 0, $37 = 0, $370 = 0, $371 = 0, $372 = 0, $373 = 0, $374 = 0, $375 = 0, $376 = 0, $377 = 0, $378 = 0, $379 = 0, $38 = 0, $380 = 0, $381 = 0, $382 = 0, $383 = 0, $384 = 0, $39 = 0, $4 = 0;
 var $40 = 0, $41 = 0, $42 = 0, $43 = 0, $44 = 0, $45 = 0, $46 = 0, $47 = 0, $48 = 0, $49 = 0, $5 = 0, $50 = 0, $51 = 0, $52 = 0, $53 = 0, $54 = 0, $55 = 0, $56 = 0, $57 = 0, $58 = 0;
 var $59 = 0, $6 = 0, $60 = 0, $61 = 0, $62 = 0, $63 = 0, $64 = 0, $65 = 0, $66 = 0, $67 = 0, $68 = 0, $69 = 0, $7 = 0, $70 = 0, $71 = 0, $72 = 0, $73 = 0, $74 = 0, $75 = 0, $76 = 0;
 var $77 = 0, $78 = 0, $79 = 0, $8 = 0, $80 = 0, $81 = 0, $82 = 0, $83 = 0, $84 = 0, $85 = 0, $86 = 0, $87 = 0, $88 = 0, $89 = 0, $9 = 0, $90 = 0, $91 = 0, $92 = 0, $93 = 0, $94 = 0;
 var $95 = 0, $96 = 0, $97 = 0, $98 = 0, $99 = 0, $n = 0, $sum2 = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 32|0;
 $1 = $adler;
 $2 = $buf;
 $3 = $len;
 $4 = $1;
 $5 = $4 >>> 16;
 $6 = $5 & 65535;
 $sum2 = $6;
 $7 = $1;
 $8 = $7 & 65535;
 $1 = $8;
 $9 = $3;
 $10 = ($9|0)==(1);
 if ($10) {
  $11 = $2;
  $12 = HEAP8[$11]|0;
  $13 = $12&255;
  $14 = $1;
  $15 = (($14) + ($13))|0;
  $1 = $15;
  $16 = $1;
  $17 = ($16>>>0)>=(65521);
  if ($17) {
   $18 = $1;
   $19 = (($18) - 65521)|0;
   $1 = $19;
  }
  $20 = $1;
  $21 = $sum2;
  $22 = (($21) + ($20))|0;
  $sum2 = $22;
  $23 = $sum2;
  $24 = ($23>>>0)>=(65521);
  if ($24) {
   $25 = $sum2;
   $26 = (($25) - 65521)|0;
   $sum2 = $26;
  }
  $27 = $1;
  $28 = $sum2;
  $29 = $28 << 16;
  $30 = $27 | $29;
  $0 = $30;
  $384 = $0;
  STACKTOP = sp;return ($384|0);
 }
 $31 = $2;
 $32 = ($31|0)==(0|0);
 if ($32) {
  $0 = 1;
  $384 = $0;
  STACKTOP = sp;return ($384|0);
 }
 $33 = $3;
 $34 = ($33>>>0)<(16);
 if ($34) {
  while(1) {
   $35 = $3;
   $36 = (($35) + -1)|0;
   $3 = $36;
   $37 = ($35|0)!=(0);
   if (!($37)) {
    break;
   }
   $38 = $2;
   $39 = (($38) + 1|0);
   $2 = $39;
   $40 = HEAP8[$38]|0;
   $41 = $40&255;
   $42 = $1;
   $43 = (($42) + ($41))|0;
   $1 = $43;
   $44 = $1;
   $45 = $sum2;
   $46 = (($45) + ($44))|0;
   $sum2 = $46;
  }
  $47 = $1;
  $48 = ($47>>>0)>=(65521);
  if ($48) {
   $49 = $1;
   $50 = (($49) - 65521)|0;
   $1 = $50;
  }
  $51 = $sum2;
  $52 = (($51>>>0) % 65521)&-1;
  $sum2 = $52;
  $53 = $1;
  $54 = $sum2;
  $55 = $54 << 16;
  $56 = $53 | $55;
  $0 = $56;
  $384 = $0;
  STACKTOP = sp;return ($384|0);
 }
 while(1) {
  $57 = $3;
  $58 = ($57>>>0)>=(5552);
  if (!($58)) {
   break;
  }
  $59 = $3;
  $60 = (($59) - 5552)|0;
  $3 = $60;
  $n = 347;
  while(1) {
   $61 = $2;
   $62 = HEAP8[$61]|0;
   $63 = $62&255;
   $64 = $1;
   $65 = (($64) + ($63))|0;
   $1 = $65;
   $66 = $1;
   $67 = $sum2;
   $68 = (($67) + ($66))|0;
   $sum2 = $68;
   $69 = $2;
   $70 = (($69) + 1|0);
   $71 = HEAP8[$70]|0;
   $72 = $71&255;
   $73 = $1;
   $74 = (($73) + ($72))|0;
   $1 = $74;
   $75 = $1;
   $76 = $sum2;
   $77 = (($76) + ($75))|0;
   $sum2 = $77;
   $78 = $2;
   $79 = (($78) + 2|0);
   $80 = HEAP8[$79]|0;
   $81 = $80&255;
   $82 = $1;
   $83 = (($82) + ($81))|0;
   $1 = $83;
   $84 = $1;
   $85 = $sum2;
   $86 = (($85) + ($84))|0;
   $sum2 = $86;
   $87 = $2;
   $88 = (($87) + 3|0);
   $89 = HEAP8[$88]|0;
   $90 = $89&255;
   $91 = $1;
   $92 = (($91) + ($90))|0;
   $1 = $92;
   $93 = $1;
   $94 = $sum2;
   $95 = (($94) + ($93))|0;
   $sum2 = $95;
   $96 = $2;
   $97 = (($96) + 4|0);
   $98 = HEAP8[$97]|0;
   $99 = $98&255;
   $100 = $1;
   $101 = (($100) + ($99))|0;
   $1 = $101;
   $102 = $1;
   $103 = $sum2;
   $104 = (($103) + ($102))|0;
   $sum2 = $104;
   $105 = $2;
   $106 = (($105) + 5|0);
   $107 = HEAP8[$106]|0;
   $108 = $107&255;
   $109 = $1;
   $110 = (($109) + ($108))|0;
   $1 = $110;
   $111 = $1;
   $112 = $sum2;
   $113 = (($112) + ($111))|0;
   $sum2 = $113;
   $114 = $2;
   $115 = (($114) + 6|0);
   $116 = HEAP8[$115]|0;
   $117 = $116&255;
   $118 = $1;
   $119 = (($118) + ($117))|0;
   $1 = $119;
   $120 = $1;
   $121 = $sum2;
   $122 = (($121) + ($120))|0;
   $sum2 = $122;
   $123 = $2;
   $124 = (($123) + 7|0);
   $125 = HEAP8[$124]|0;
   $126 = $125&255;
   $127 = $1;
   $128 = (($127) + ($126))|0;
   $1 = $128;
   $129 = $1;
   $130 = $sum2;
   $131 = (($130) + ($129))|0;
   $sum2 = $131;
   $132 = $2;
   $133 = (($132) + 8|0);
   $134 = HEAP8[$133]|0;
   $135 = $134&255;
   $136 = $1;
   $137 = (($136) + ($135))|0;
   $1 = $137;
   $138 = $1;
   $139 = $sum2;
   $140 = (($139) + ($138))|0;
   $sum2 = $140;
   $141 = $2;
   $142 = (($141) + 9|0);
   $143 = HEAP8[$142]|0;
   $144 = $143&255;
   $145 = $1;
   $146 = (($145) + ($144))|0;
   $1 = $146;
   $147 = $1;
   $148 = $sum2;
   $149 = (($148) + ($147))|0;
   $sum2 = $149;
   $150 = $2;
   $151 = (($150) + 10|0);
   $152 = HEAP8[$151]|0;
   $153 = $152&255;
   $154 = $1;
   $155 = (($154) + ($153))|0;
   $1 = $155;
   $156 = $1;
   $157 = $sum2;
   $158 = (($157) + ($156))|0;
   $sum2 = $158;
   $159 = $2;
   $160 = (($159) + 11|0);
   $161 = HEAP8[$160]|0;
   $162 = $161&255;
   $163 = $1;
   $164 = (($163) + ($162))|0;
   $1 = $164;
   $165 = $1;
   $166 = $sum2;
   $167 = (($166) + ($165))|0;
   $sum2 = $167;
   $168 = $2;
   $169 = (($168) + 12|0);
   $170 = HEAP8[$169]|0;
   $171 = $170&255;
   $172 = $1;
   $173 = (($172) + ($171))|0;
   $1 = $173;
   $174 = $1;
   $175 = $sum2;
   $176 = (($175) + ($174))|0;
   $sum2 = $176;
   $177 = $2;
   $178 = (($177) + 13|0);
   $179 = HEAP8[$178]|0;
   $180 = $179&255;
   $181 = $1;
   $182 = (($181) + ($180))|0;
   $1 = $182;
   $183 = $1;
   $184 = $sum2;
   $185 = (($184) + ($183))|0;
   $sum2 = $185;
   $186 = $2;
   $187 = (($186) + 14|0);
   $188 = HEAP8[$187]|0;
   $189 = $188&255;
   $190 = $1;
   $191 = (($190) + ($189))|0;
   $1 = $191;
   $192 = $1;
   $193 = $sum2;
   $194 = (($193) + ($192))|0;
   $sum2 = $194;
   $195 = $2;
   $196 = (($195) + 15|0);
   $197 = HEAP8[$196]|0;
   $198 = $197&255;
   $199 = $1;
   $200 = (($199) + ($198))|0;
   $1 = $200;
   $201 = $1;
   $202 = $sum2;
   $203 = (($202) + ($201))|0;
   $sum2 = $203;
   $204 = $2;
   $205 = (($204) + 16|0);
   $2 = $205;
   $206 = $n;
   $207 = (($206) + -1)|0;
   $n = $207;
   $208 = ($207|0)!=(0);
   if (!($208)) {
    break;
   }
  }
  $209 = $1;
  $210 = (($209>>>0) % 65521)&-1;
  $1 = $210;
  $211 = $sum2;
  $212 = (($211>>>0) % 65521)&-1;
  $sum2 = $212;
 }
 $213 = $3;
 $214 = ($213|0)!=(0);
 if ($214) {
  while(1) {
   $215 = $3;
   $216 = ($215>>>0)>=(16);
   if (!($216)) {
    break;
   }
   $217 = $3;
   $218 = (($217) - 16)|0;
   $3 = $218;
   $219 = $2;
   $220 = HEAP8[$219]|0;
   $221 = $220&255;
   $222 = $1;
   $223 = (($222) + ($221))|0;
   $1 = $223;
   $224 = $1;
   $225 = $sum2;
   $226 = (($225) + ($224))|0;
   $sum2 = $226;
   $227 = $2;
   $228 = (($227) + 1|0);
   $229 = HEAP8[$228]|0;
   $230 = $229&255;
   $231 = $1;
   $232 = (($231) + ($230))|0;
   $1 = $232;
   $233 = $1;
   $234 = $sum2;
   $235 = (($234) + ($233))|0;
   $sum2 = $235;
   $236 = $2;
   $237 = (($236) + 2|0);
   $238 = HEAP8[$237]|0;
   $239 = $238&255;
   $240 = $1;
   $241 = (($240) + ($239))|0;
   $1 = $241;
   $242 = $1;
   $243 = $sum2;
   $244 = (($243) + ($242))|0;
   $sum2 = $244;
   $245 = $2;
   $246 = (($245) + 3|0);
   $247 = HEAP8[$246]|0;
   $248 = $247&255;
   $249 = $1;
   $250 = (($249) + ($248))|0;
   $1 = $250;
   $251 = $1;
   $252 = $sum2;
   $253 = (($252) + ($251))|0;
   $sum2 = $253;
   $254 = $2;
   $255 = (($254) + 4|0);
   $256 = HEAP8[$255]|0;
   $257 = $256&255;
   $258 = $1;
   $259 = (($258) + ($257))|0;
   $1 = $259;
   $260 = $1;
   $261 = $sum2;
   $262 = (($261) + ($260))|0;
   $sum2 = $262;
   $263 = $2;
   $264 = (($263) + 5|0);
   $265 = HEAP8[$264]|0;
   $266 = $265&255;
   $267 = $1;
   $268 = (($267) + ($266))|0;
   $1 = $268;
   $269 = $1;
   $270 = $sum2;
   $271 = (($270) + ($269))|0;
   $sum2 = $271;
   $272 = $2;
   $273 = (($272) + 6|0);
   $274 = HEAP8[$273]|0;
   $275 = $274&255;
   $276 = $1;
   $277 = (($276) + ($275))|0;
   $1 = $277;
   $278 = $1;
   $279 = $sum2;
   $280 = (($279) + ($278))|0;
   $sum2 = $280;
   $281 = $2;
   $282 = (($281) + 7|0);
   $283 = HEAP8[$282]|0;
   $284 = $283&255;
   $285 = $1;
   $286 = (($285) + ($284))|0;
   $1 = $286;
   $287 = $1;
   $288 = $sum2;
   $289 = (($288) + ($287))|0;
   $sum2 = $289;
   $290 = $2;
   $291 = (($290) + 8|0);
   $292 = HEAP8[$291]|0;
   $293 = $292&255;
   $294 = $1;
   $295 = (($294) + ($293))|0;
   $1 = $295;
   $296 = $1;
   $297 = $sum2;
   $298 = (($297) + ($296))|0;
   $sum2 = $298;
   $299 = $2;
   $300 = (($299) + 9|0);
   $301 = HEAP8[$300]|0;
   $302 = $301&255;
   $303 = $1;
   $304 = (($303) + ($302))|0;
   $1 = $304;
   $305 = $1;
   $306 = $sum2;
   $307 = (($306) + ($305))|0;
   $sum2 = $307;
   $308 = $2;
   $309 = (($308) + 10|0);
   $310 = HEAP8[$309]|0;
   $311 = $310&255;
   $312 = $1;
   $313 = (($312) + ($311))|0;
   $1 = $313;
   $314 = $1;
   $315 = $sum2;
   $316 = (($315) + ($314))|0;
   $sum2 = $316;
   $317 = $2;
   $318 = (($317) + 11|0);
   $319 = HEAP8[$318]|0;
   $320 = $319&255;
   $321 = $1;
   $322 = (($321) + ($320))|0;
   $1 = $322;
   $323 = $1;
   $324 = $sum2;
   $325 = (($324) + ($323))|0;
   $sum2 = $325;
   $326 = $2;
   $327 = (($326) + 12|0);
   $328 = HEAP8[$327]|0;
   $329 = $328&255;
   $330 = $1;
   $331 = (($330) + ($329))|0;
   $1 = $331;
   $332 = $1;
   $333 = $sum2;
   $334 = (($333) + ($332))|0;
   $sum2 = $334;
   $335 = $2;
   $336 = (($335) + 13|0);
   $337 = HEAP8[$336]|0;
   $338 = $337&255;
   $339 = $1;
   $340 = (($339) + ($338))|0;
   $1 = $340;
   $341 = $1;
   $342 = $sum2;
   $343 = (($342) + ($341))|0;
   $sum2 = $343;
   $344 = $2;
   $345 = (($344) + 14|0);
   $346 = HEAP8[$345]|0;
   $347 = $346&255;
   $348 = $1;
   $349 = (($348) + ($347))|0;
   $1 = $349;
   $350 = $1;
   $351 = $sum2;
   $352 = (($351) + ($350))|0;
   $sum2 = $352;
   $353 = $2;
   $354 = (($353) + 15|0);
   $355 = HEAP8[$354]|0;
   $356 = $355&255;
   $357 = $1;
   $358 = (($357) + ($356))|0;
   $1 = $358;
   $359 = $1;
   $360 = $sum2;
   $361 = (($360) + ($359))|0;
   $sum2 = $361;
   $362 = $2;
   $363 = (($362) + 16|0);
   $2 = $363;
  }
  while(1) {
   $364 = $3;
   $365 = (($364) + -1)|0;
   $3 = $365;
   $366 = ($364|0)!=(0);
   if (!($366)) {
    break;
   }
   $367 = $2;
   $368 = (($367) + 1|0);
   $2 = $368;
   $369 = HEAP8[$367]|0;
   $370 = $369&255;
   $371 = $1;
   $372 = (($371) + ($370))|0;
   $1 = $372;
   $373 = $1;
   $374 = $sum2;
   $375 = (($374) + ($373))|0;
   $sum2 = $375;
  }
  $376 = $1;
  $377 = (($376>>>0) % 65521)&-1;
  $1 = $377;
  $378 = $sum2;
  $379 = (($378>>>0) % 65521)&-1;
  $sum2 = $379;
 }
 $380 = $1;
 $381 = $sum2;
 $382 = $381 << 16;
 $383 = $380 | $382;
 $0 = $383;
 $384 = $0;
 STACKTOP = sp;return ($384|0);
}
function _inflate_fast($strm,$start) {
 $strm = $strm|0;
 $start = $start|0;
 var $0 = 0, $1 = 0, $10 = 0, $100 = 0, $101 = 0, $102 = 0, $103 = 0, $104 = 0, $105 = 0, $106 = 0, $107 = 0, $108 = 0, $109 = 0, $11 = 0, $110 = 0, $111 = 0, $112 = 0, $113 = 0, $114 = 0, $115 = 0;
 var $116 = 0, $117 = 0, $118 = 0, $119 = 0, $12 = 0, $120 = 0, $121 = 0, $122 = 0, $123 = 0, $124 = 0, $125 = 0, $126 = 0, $127 = 0, $128 = 0, $129 = 0, $13 = 0, $130 = 0, $131 = 0, $132 = 0, $133 = 0;
 var $134 = 0, $135 = 0, $136 = 0, $137 = 0, $138 = 0, $139 = 0, $14 = 0, $140 = 0, $141 = 0, $142 = 0, $143 = 0, $144 = 0, $145 = 0, $146 = 0, $147 = 0, $148 = 0, $149 = 0, $15 = 0, $150 = 0, $151 = 0;
 var $152 = 0, $153 = 0, $154 = 0, $155 = 0, $156 = 0, $157 = 0, $158 = 0, $159 = 0, $16 = 0, $160 = 0, $161 = 0, $162 = 0, $163 = 0, $164 = 0, $165 = 0, $166 = 0, $167 = 0, $168 = 0, $169 = 0, $17 = 0;
 var $170 = 0, $171 = 0, $172 = 0, $173 = 0, $174 = 0, $175 = 0, $176 = 0, $177 = 0, $178 = 0, $179 = 0, $18 = 0, $180 = 0, $181 = 0, $182 = 0, $183 = 0, $184 = 0, $185 = 0, $186 = 0, $187 = 0, $188 = 0;
 var $189 = 0, $19 = 0, $190 = 0, $191 = 0, $192 = 0, $193 = 0, $194 = 0, $195 = 0, $196 = 0, $197 = 0, $198 = 0, $199 = 0, $2 = 0, $20 = 0, $200 = 0, $201 = 0, $202 = 0, $203 = 0, $204 = 0, $205 = 0;
 var $206 = 0, $207 = 0, $208 = 0, $209 = 0, $21 = 0, $210 = 0, $211 = 0, $212 = 0, $213 = 0, $214 = 0, $215 = 0, $216 = 0, $217 = 0, $218 = 0, $219 = 0, $22 = 0, $220 = 0, $221 = 0, $222 = 0, $223 = 0;
 var $224 = 0, $225 = 0, $226 = 0, $227 = 0, $228 = 0, $229 = 0, $23 = 0, $230 = 0, $231 = 0, $232 = 0, $233 = 0, $234 = 0, $235 = 0, $236 = 0, $237 = 0, $238 = 0, $239 = 0, $24 = 0, $240 = 0, $241 = 0;
 var $242 = 0, $243 = 0, $244 = 0, $245 = 0, $246 = 0, $247 = 0, $248 = 0, $249 = 0, $25 = 0, $250 = 0, $251 = 0, $252 = 0, $253 = 0, $254 = 0, $255 = 0, $256 = 0, $257 = 0, $258 = 0, $259 = 0, $26 = 0;
 var $260 = 0, $261 = 0, $262 = 0, $263 = 0, $264 = 0, $265 = 0, $266 = 0, $267 = 0, $268 = 0, $269 = 0, $27 = 0, $270 = 0, $271 = 0, $272 = 0, $273 = 0, $274 = 0, $275 = 0, $276 = 0, $277 = 0, $278 = 0;
 var $279 = 0, $28 = 0, $280 = 0, $281 = 0, $282 = 0, $283 = 0, $284 = 0, $285 = 0, $286 = 0, $287 = 0, $288 = 0, $289 = 0, $29 = 0, $290 = 0, $291 = 0, $292 = 0, $293 = 0, $294 = 0, $295 = 0, $296 = 0;
 var $297 = 0, $298 = 0, $299 = 0, $3 = 0, $30 = 0, $300 = 0, $301 = 0, $302 = 0, $303 = 0, $304 = 0, $305 = 0, $306 = 0, $307 = 0, $308 = 0, $309 = 0, $31 = 0, $310 = 0, $311 = 0, $312 = 0, $313 = 0;
 var $314 = 0, $315 = 0, $316 = 0, $317 = 0, $318 = 0, $319 = 0, $32 = 0, $320 = 0, $321 = 0, $322 = 0, $323 = 0, $324 = 0, $325 = 0, $326 = 0, $327 = 0, $328 = 0, $329 = 0, $33 = 0, $330 = 0, $331 = 0;
 var $332 = 0, $333 = 0, $334 = 0, $335 = 0, $336 = 0, $337 = 0, $338 = 0, $339 = 0, $34 = 0, $340 = 0, $341 = 0, $342 = 0, $343 = 0, $344 = 0, $345 = 0, $346 = 0, $347 = 0, $348 = 0, $349 = 0, $35 = 0;
 var $350 = 0, $351 = 0, $352 = 0, $353 = 0, $354 = 0, $355 = 0, $356 = 0, $357 = 0, $358 = 0, $359 = 0, $36 = 0, $360 = 0, $361 = 0, $362 = 0, $363 = 0, $364 = 0, $365 = 0, $366 = 0, $367 = 0, $368 = 0;
 var $369 = 0, $37 = 0, $370 = 0, $371 = 0, $372 = 0, $373 = 0, $374 = 0, $375 = 0, $376 = 0, $377 = 0, $378 = 0, $379 = 0, $38 = 0, $380 = 0, $381 = 0, $382 = 0, $383 = 0, $384 = 0, $385 = 0, $386 = 0;
 var $387 = 0, $388 = 0, $389 = 0, $39 = 0, $390 = 0, $391 = 0, $392 = 0, $393 = 0, $394 = 0, $395 = 0, $396 = 0, $397 = 0, $398 = 0, $399 = 0, $4 = 0, $40 = 0, $400 = 0, $401 = 0, $402 = 0, $403 = 0;
 var $404 = 0, $405 = 0, $406 = 0, $407 = 0, $408 = 0, $409 = 0, $41 = 0, $410 = 0, $411 = 0, $412 = 0, $413 = 0, $414 = 0, $415 = 0, $416 = 0, $417 = 0, $418 = 0, $419 = 0, $42 = 0, $420 = 0, $421 = 0;
 var $422 = 0, $423 = 0, $424 = 0, $425 = 0, $426 = 0, $427 = 0, $428 = 0, $429 = 0, $43 = 0, $430 = 0, $431 = 0, $432 = 0, $433 = 0, $434 = 0, $435 = 0, $436 = 0, $437 = 0, $438 = 0, $439 = 0, $44 = 0;
 var $440 = 0, $441 = 0, $442 = 0, $443 = 0, $444 = 0, $445 = 0, $446 = 0, $447 = 0, $448 = 0, $449 = 0, $45 = 0, $450 = 0, $451 = 0, $452 = 0, $453 = 0, $454 = 0, $455 = 0, $456 = 0, $457 = 0, $458 = 0;
 var $459 = 0, $46 = 0, $460 = 0, $461 = 0, $462 = 0, $463 = 0, $464 = 0, $465 = 0, $466 = 0, $467 = 0, $468 = 0, $469 = 0, $47 = 0, $470 = 0, $471 = 0, $472 = 0, $473 = 0, $474 = 0, $475 = 0, $476 = 0;
 var $477 = 0, $478 = 0, $479 = 0, $48 = 0, $480 = 0, $481 = 0, $482 = 0, $483 = 0, $484 = 0, $485 = 0, $486 = 0, $487 = 0, $488 = 0, $489 = 0, $49 = 0, $490 = 0, $491 = 0, $492 = 0, $493 = 0, $494 = 0;
 var $495 = 0, $496 = 0, $497 = 0, $498 = 0, $499 = 0, $5 = 0, $50 = 0, $500 = 0, $501 = 0, $502 = 0, $503 = 0, $504 = 0, $505 = 0, $506 = 0, $507 = 0, $508 = 0, $509 = 0, $51 = 0, $510 = 0, $511 = 0;
 var $512 = 0, $513 = 0, $514 = 0, $515 = 0, $516 = 0, $517 = 0, $518 = 0, $519 = 0, $52 = 0, $520 = 0, $521 = 0, $522 = 0, $523 = 0, $524 = 0, $525 = 0, $526 = 0, $527 = 0, $528 = 0, $529 = 0, $53 = 0;
 var $54 = 0, $55 = 0, $56 = 0, $57 = 0, $58 = 0, $59 = 0, $6 = 0, $60 = 0, $61 = 0, $62 = 0, $63 = 0, $64 = 0, $65 = 0, $66 = 0, $67 = 0, $68 = 0, $69 = 0, $7 = 0, $70 = 0, $71 = 0;
 var $72 = 0, $73 = 0, $74 = 0, $75 = 0, $76 = 0, $77 = 0, $78 = 0, $79 = 0, $8 = 0, $80 = 0, $81 = 0, $82 = 0, $83 = 0, $84 = 0, $85 = 0, $86 = 0, $87 = 0, $88 = 0, $89 = 0, $9 = 0;
 var $90 = 0, $91 = 0, $92 = 0, $93 = 0, $94 = 0, $95 = 0, $96 = 0, $97 = 0, $98 = 0, $99 = 0, $beg = 0, $bits = 0, $dcode = 0, $dist = 0, $dmask = 0, $end = 0, $from = 0, $here = 0, $hold = 0, $in = 0;
 var $last = 0, $lcode = 0, $len = 0, $lmask = 0, $op = 0, $out = 0, $state = 0, $whave = 0, $window = 0, $wnext = 0, $wsize = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 96|0;
 $here = sp + 88|0;
 $0 = $strm;
 $1 = $start;
 $2 = $0;
 $3 = (($2) + 28|0);
 $4 = HEAP32[$3>>2]|0;
 $state = $4;
 $5 = $0;
 $6 = HEAP32[$5>>2]|0;
 $7 = (($6) + -1|0);
 $in = $7;
 $8 = $in;
 $9 = $0;
 $10 = (($9) + 4|0);
 $11 = HEAP32[$10>>2]|0;
 $12 = (($11) - 5)|0;
 $13 = (($8) + ($12)|0);
 $last = $13;
 $14 = $0;
 $15 = (($14) + 12|0);
 $16 = HEAP32[$15>>2]|0;
 $17 = (($16) + -1|0);
 $out = $17;
 $18 = $out;
 $19 = $1;
 $20 = $0;
 $21 = (($20) + 16|0);
 $22 = HEAP32[$21>>2]|0;
 $23 = (($19) - ($22))|0;
 $24 = (0 - ($23))|0;
 $25 = (($18) + ($24)|0);
 $beg = $25;
 $26 = $out;
 $27 = $0;
 $28 = (($27) + 16|0);
 $29 = HEAP32[$28>>2]|0;
 $30 = (($29) - 257)|0;
 $31 = (($26) + ($30)|0);
 $end = $31;
 $32 = $state;
 $33 = (($32) + 40|0);
 $34 = HEAP32[$33>>2]|0;
 $wsize = $34;
 $35 = $state;
 $36 = (($35) + 44|0);
 $37 = HEAP32[$36>>2]|0;
 $whave = $37;
 $38 = $state;
 $39 = (($38) + 48|0);
 $40 = HEAP32[$39>>2]|0;
 $wnext = $40;
 $41 = $state;
 $42 = (($41) + 52|0);
 $43 = HEAP32[$42>>2]|0;
 $window = $43;
 $44 = $state;
 $45 = (($44) + 56|0);
 $46 = HEAP32[$45>>2]|0;
 $hold = $46;
 $47 = $state;
 $48 = (($47) + 60|0);
 $49 = HEAP32[$48>>2]|0;
 $bits = $49;
 $50 = $state;
 $51 = (($50) + 76|0);
 $52 = HEAP32[$51>>2]|0;
 $lcode = $52;
 $53 = $state;
 $54 = (($53) + 80|0);
 $55 = HEAP32[$54>>2]|0;
 $dcode = $55;
 $56 = $state;
 $57 = (($56) + 84|0);
 $58 = HEAP32[$57>>2]|0;
 $59 = 1 << $58;
 $60 = (($59) - 1)|0;
 $lmask = $60;
 $61 = $state;
 $62 = (($61) + 88|0);
 $63 = HEAP32[$62>>2]|0;
 $64 = 1 << $63;
 $65 = (($64) - 1)|0;
 $dmask = $65;
 L1: while(1) {
  $66 = $bits;
  $67 = ($66>>>0)<(15);
  if ($67) {
   $68 = $in;
   $69 = (($68) + 1|0);
   $in = $69;
   $70 = HEAP8[$69]|0;
   $71 = $70&255;
   $72 = $bits;
   $73 = $71 << $72;
   $74 = $hold;
   $75 = (($74) + ($73))|0;
   $hold = $75;
   $76 = $bits;
   $77 = (($76) + 8)|0;
   $bits = $77;
   $78 = $in;
   $79 = (($78) + 1|0);
   $in = $79;
   $80 = HEAP8[$79]|0;
   $81 = $80&255;
   $82 = $bits;
   $83 = $81 << $82;
   $84 = $hold;
   $85 = (($84) + ($83))|0;
   $hold = $85;
   $86 = $bits;
   $87 = (($86) + 8)|0;
   $bits = $87;
  }
  $88 = $hold;
  $89 = $lmask;
  $90 = $88 & $89;
  $91 = $lcode;
  $92 = (($91) + ($90<<2)|0);
  ;HEAP16[$here+0>>1]=HEAP16[$92+0>>1]|0;HEAP16[$here+2>>1]=HEAP16[$92+2>>1]|0;
  while(1) {
   $93 = (($here) + 1|0);
   $94 = HEAP8[$93]|0;
   $95 = $94&255;
   $op = $95;
   $96 = $op;
   $97 = $hold;
   $98 = $97 >>> $96;
   $hold = $98;
   $99 = $op;
   $100 = $bits;
   $101 = (($100) - ($99))|0;
   $bits = $101;
   $102 = HEAP8[$here]|0;
   $103 = $102&255;
   $op = $103;
   $104 = $op;
   $105 = ($104|0)==(0);
   if ($105) {
    label = 6;
    break;
   }
   $111 = $op;
   $112 = $111 & 16;
   $113 = ($112|0)!=(0);
   if ($113) {
    label = 8;
    break;
   }
   $438 = $op;
   $439 = $438 & 64;
   $440 = ($439|0)==(0);
   if (!($440)) {
    label = 74;
    break L1;
   }
   $441 = (($here) + 2|0);
   $442 = HEAP16[$441>>1]|0;
   $443 = $442&65535;
   $444 = $hold;
   $445 = $op;
   $446 = 1 << $445;
   $447 = (($446) - 1)|0;
   $448 = $444 & $447;
   $449 = (($443) + ($448))|0;
   $450 = $lcode;
   $451 = (($450) + ($449<<2)|0);
   ;HEAP16[$here+0>>1]=HEAP16[$451+0>>1]|0;HEAP16[$here+2>>1]=HEAP16[$451+2>>1]|0;
  }
  if ((label|0) == 6) {
   label = 0;
   $106 = (($here) + 2|0);
   $107 = HEAP16[$106>>1]|0;
   $108 = $107&255;
   $109 = $out;
   $110 = (($109) + 1|0);
   $out = $110;
   HEAP8[$110] = $108;
  }
  else if ((label|0) == 8) {
   label = 0;
   $114 = (($here) + 2|0);
   $115 = HEAP16[$114>>1]|0;
   $116 = $115&65535;
   $len = $116;
   $117 = $op;
   $118 = $117 & 15;
   $op = $118;
   $119 = $op;
   $120 = ($119|0)!=(0);
   if ($120) {
    $121 = $bits;
    $122 = $op;
    $123 = ($121>>>0)<($122>>>0);
    if ($123) {
     $124 = $in;
     $125 = (($124) + 1|0);
     $in = $125;
     $126 = HEAP8[$125]|0;
     $127 = $126&255;
     $128 = $bits;
     $129 = $127 << $128;
     $130 = $hold;
     $131 = (($130) + ($129))|0;
     $hold = $131;
     $132 = $bits;
     $133 = (($132) + 8)|0;
     $bits = $133;
    }
    $134 = $hold;
    $135 = $op;
    $136 = 1 << $135;
    $137 = (($136) - 1)|0;
    $138 = $134 & $137;
    $139 = $len;
    $140 = (($139) + ($138))|0;
    $len = $140;
    $141 = $op;
    $142 = $hold;
    $143 = $142 >>> $141;
    $hold = $143;
    $144 = $op;
    $145 = $bits;
    $146 = (($145) - ($144))|0;
    $bits = $146;
   }
   $147 = $bits;
   $148 = ($147>>>0)<(15);
   if ($148) {
    $149 = $in;
    $150 = (($149) + 1|0);
    $in = $150;
    $151 = HEAP8[$150]|0;
    $152 = $151&255;
    $153 = $bits;
    $154 = $152 << $153;
    $155 = $hold;
    $156 = (($155) + ($154))|0;
    $hold = $156;
    $157 = $bits;
    $158 = (($157) + 8)|0;
    $bits = $158;
    $159 = $in;
    $160 = (($159) + 1|0);
    $in = $160;
    $161 = HEAP8[$160]|0;
    $162 = $161&255;
    $163 = $bits;
    $164 = $162 << $163;
    $165 = $hold;
    $166 = (($165) + ($164))|0;
    $hold = $166;
    $167 = $bits;
    $168 = (($167) + 8)|0;
    $bits = $168;
   }
   $169 = $hold;
   $170 = $dmask;
   $171 = $169 & $170;
   $172 = $dcode;
   $173 = (($172) + ($171<<2)|0);
   ;HEAP16[$here+0>>1]=HEAP16[$173+0>>1]|0;HEAP16[$here+2>>1]=HEAP16[$173+2>>1]|0;
   while(1) {
    $174 = (($here) + 1|0);
    $175 = HEAP8[$174]|0;
    $176 = $175&255;
    $op = $176;
    $177 = $op;
    $178 = $hold;
    $179 = $178 >>> $177;
    $hold = $179;
    $180 = $op;
    $181 = $bits;
    $182 = (($181) - ($180))|0;
    $bits = $182;
    $183 = HEAP8[$here]|0;
    $184 = $183&255;
    $op = $184;
    $185 = $op;
    $186 = $185 & 16;
    $187 = ($186|0)!=(0);
    if ($187) {
     break;
    }
    $421 = $op;
    $422 = $421 & 64;
    $423 = ($422|0)==(0);
    if (!($423)) {
     label = 70;
     break L1;
    }
    $424 = (($here) + 2|0);
    $425 = HEAP16[$424>>1]|0;
    $426 = $425&65535;
    $427 = $hold;
    $428 = $op;
    $429 = 1 << $428;
    $430 = (($429) - 1)|0;
    $431 = $427 & $430;
    $432 = (($426) + ($431))|0;
    $433 = $dcode;
    $434 = (($433) + ($432<<2)|0);
    ;HEAP16[$here+0>>1]=HEAP16[$434+0>>1]|0;HEAP16[$here+2>>1]=HEAP16[$434+2>>1]|0;
   }
   $188 = (($here) + 2|0);
   $189 = HEAP16[$188>>1]|0;
   $190 = $189&65535;
   $dist = $190;
   $191 = $op;
   $192 = $191 & 15;
   $op = $192;
   $193 = $bits;
   $194 = $op;
   $195 = ($193>>>0)<($194>>>0);
   if ($195) {
    $196 = $in;
    $197 = (($196) + 1|0);
    $in = $197;
    $198 = HEAP8[$197]|0;
    $199 = $198&255;
    $200 = $bits;
    $201 = $199 << $200;
    $202 = $hold;
    $203 = (($202) + ($201))|0;
    $hold = $203;
    $204 = $bits;
    $205 = (($204) + 8)|0;
    $bits = $205;
    $206 = $bits;
    $207 = $op;
    $208 = ($206>>>0)<($207>>>0);
    if ($208) {
     $209 = $in;
     $210 = (($209) + 1|0);
     $in = $210;
     $211 = HEAP8[$210]|0;
     $212 = $211&255;
     $213 = $bits;
     $214 = $212 << $213;
     $215 = $hold;
     $216 = (($215) + ($214))|0;
     $hold = $216;
     $217 = $bits;
     $218 = (($217) + 8)|0;
     $bits = $218;
    }
   }
   $219 = $hold;
   $220 = $op;
   $221 = 1 << $220;
   $222 = (($221) - 1)|0;
   $223 = $219 & $222;
   $224 = $dist;
   $225 = (($224) + ($223))|0;
   $dist = $225;
   $226 = $op;
   $227 = $hold;
   $228 = $227 >>> $226;
   $hold = $228;
   $229 = $op;
   $230 = $bits;
   $231 = (($230) - ($229))|0;
   $bits = $231;
   $232 = $out;
   $233 = $beg;
   $234 = $232;
   $235 = $233;
   $236 = (($234) - ($235))|0;
   $op = $236;
   $237 = $dist;
   $238 = $op;
   $239 = ($237>>>0)>($238>>>0);
   if ($239) {
    $240 = $dist;
    $241 = $op;
    $242 = (($240) - ($241))|0;
    $op = $242;
    $243 = $op;
    $244 = $whave;
    $245 = ($243>>>0)>($244>>>0);
    if ($245) {
     $246 = $state;
     $247 = (($246) + 7104|0);
     $248 = HEAP32[$247>>2]|0;
     $249 = ($248|0)!=(0);
     if ($249) {
      label = 23;
      break;
     }
    }
    $253 = $window;
    $254 = (($253) + -1|0);
    $from = $254;
    $255 = $wnext;
    $256 = ($255|0)==(0);
    if ($256) {
     $257 = $wsize;
     $258 = $op;
     $259 = (($257) - ($258))|0;
     $260 = $from;
     $261 = (($260) + ($259)|0);
     $from = $261;
     $262 = $op;
     $263 = $len;
     $264 = ($262>>>0)<($263>>>0);
     if ($264) {
      $265 = $op;
      $266 = $len;
      $267 = (($266) - ($265))|0;
      $len = $267;
      while(1) {
       $268 = $from;
       $269 = (($268) + 1|0);
       $from = $269;
       $270 = HEAP8[$269]|0;
       $271 = $out;
       $272 = (($271) + 1|0);
       $out = $272;
       HEAP8[$272] = $270;
       $273 = $op;
       $274 = (($273) + -1)|0;
       $op = $274;
       $275 = ($274|0)!=(0);
       if (!($275)) {
        break;
       }
      }
      $276 = $out;
      $277 = $dist;
      $278 = (0 - ($277))|0;
      $279 = (($276) + ($278)|0);
      $from = $279;
     }
    } else {
     $280 = $wnext;
     $281 = $op;
     $282 = ($280>>>0)<($281>>>0);
     if ($282) {
      $283 = $wsize;
      $284 = $wnext;
      $285 = (($283) + ($284))|0;
      $286 = $op;
      $287 = (($285) - ($286))|0;
      $288 = $from;
      $289 = (($288) + ($287)|0);
      $from = $289;
      $290 = $wnext;
      $291 = $op;
      $292 = (($291) - ($290))|0;
      $op = $292;
      $293 = $op;
      $294 = $len;
      $295 = ($293>>>0)<($294>>>0);
      if ($295) {
       $296 = $op;
       $297 = $len;
       $298 = (($297) - ($296))|0;
       $len = $298;
       while(1) {
        $299 = $from;
        $300 = (($299) + 1|0);
        $from = $300;
        $301 = HEAP8[$300]|0;
        $302 = $out;
        $303 = (($302) + 1|0);
        $out = $303;
        HEAP8[$303] = $301;
        $304 = $op;
        $305 = (($304) + -1)|0;
        $op = $305;
        $306 = ($305|0)!=(0);
        if (!($306)) {
         break;
        }
       }
       $307 = $window;
       $308 = (($307) + -1|0);
       $from = $308;
       $309 = $wnext;
       $310 = $len;
       $311 = ($309>>>0)<($310>>>0);
       if ($311) {
        $312 = $wnext;
        $op = $312;
        $313 = $op;
        $314 = $len;
        $315 = (($314) - ($313))|0;
        $len = $315;
        while(1) {
         $316 = $from;
         $317 = (($316) + 1|0);
         $from = $317;
         $318 = HEAP8[$317]|0;
         $319 = $out;
         $320 = (($319) + 1|0);
         $out = $320;
         HEAP8[$320] = $318;
         $321 = $op;
         $322 = (($321) + -1)|0;
         $op = $322;
         $323 = ($322|0)!=(0);
         if (!($323)) {
          break;
         }
        }
        $324 = $out;
        $325 = $dist;
        $326 = (0 - ($325))|0;
        $327 = (($324) + ($326)|0);
        $from = $327;
       }
      }
     } else {
      $328 = $wnext;
      $329 = $op;
      $330 = (($328) - ($329))|0;
      $331 = $from;
      $332 = (($331) + ($330)|0);
      $from = $332;
      $333 = $op;
      $334 = $len;
      $335 = ($333>>>0)<($334>>>0);
      if ($335) {
       $336 = $op;
       $337 = $len;
       $338 = (($337) - ($336))|0;
       $len = $338;
       while(1) {
        $339 = $from;
        $340 = (($339) + 1|0);
        $from = $340;
        $341 = HEAP8[$340]|0;
        $342 = $out;
        $343 = (($342) + 1|0);
        $out = $343;
        HEAP8[$343] = $341;
        $344 = $op;
        $345 = (($344) + -1)|0;
        $op = $345;
        $346 = ($345|0)!=(0);
        if (!($346)) {
         break;
        }
       }
       $347 = $out;
       $348 = $dist;
       $349 = (0 - ($348))|0;
       $350 = (($347) + ($349)|0);
       $from = $350;
      }
     }
    }
    while(1) {
     $351 = $len;
     $352 = ($351>>>0)>(2);
     if (!($352)) {
      break;
     }
     $353 = $from;
     $354 = (($353) + 1|0);
     $from = $354;
     $355 = HEAP8[$354]|0;
     $356 = $out;
     $357 = (($356) + 1|0);
     $out = $357;
     HEAP8[$357] = $355;
     $358 = $from;
     $359 = (($358) + 1|0);
     $from = $359;
     $360 = HEAP8[$359]|0;
     $361 = $out;
     $362 = (($361) + 1|0);
     $out = $362;
     HEAP8[$362] = $360;
     $363 = $from;
     $364 = (($363) + 1|0);
     $from = $364;
     $365 = HEAP8[$364]|0;
     $366 = $out;
     $367 = (($366) + 1|0);
     $out = $367;
     HEAP8[$367] = $365;
     $368 = $len;
     $369 = (($368) - 3)|0;
     $len = $369;
    }
    $370 = $len;
    $371 = ($370|0)!=(0);
    if ($371) {
     $372 = $from;
     $373 = (($372) + 1|0);
     $from = $373;
     $374 = HEAP8[$373]|0;
     $375 = $out;
     $376 = (($375) + 1|0);
     $out = $376;
     HEAP8[$376] = $374;
     $377 = $len;
     $378 = ($377>>>0)>(1);
     if ($378) {
      $379 = $from;
      $380 = (($379) + 1|0);
      $from = $380;
      $381 = HEAP8[$380]|0;
      $382 = $out;
      $383 = (($382) + 1|0);
      $out = $383;
      HEAP8[$383] = $381;
     }
    }
   } else {
    $384 = $out;
    $385 = $dist;
    $386 = (0 - ($385))|0;
    $387 = (($384) + ($386)|0);
    $from = $387;
    while(1) {
     $388 = $from;
     $389 = (($388) + 1|0);
     $from = $389;
     $390 = HEAP8[$389]|0;
     $391 = $out;
     $392 = (($391) + 1|0);
     $out = $392;
     HEAP8[$392] = $390;
     $393 = $from;
     $394 = (($393) + 1|0);
     $from = $394;
     $395 = HEAP8[$394]|0;
     $396 = $out;
     $397 = (($396) + 1|0);
     $out = $397;
     HEAP8[$397] = $395;
     $398 = $from;
     $399 = (($398) + 1|0);
     $from = $399;
     $400 = HEAP8[$399]|0;
     $401 = $out;
     $402 = (($401) + 1|0);
     $out = $402;
     HEAP8[$402] = $400;
     $403 = $len;
     $404 = (($403) - 3)|0;
     $len = $404;
     $405 = $len;
     $406 = ($405>>>0)>(2);
     if (!($406)) {
      break;
     }
    }
    $407 = $len;
    $408 = ($407|0)!=(0);
    if ($408) {
     $409 = $from;
     $410 = (($409) + 1|0);
     $from = $410;
     $411 = HEAP8[$410]|0;
     $412 = $out;
     $413 = (($412) + 1|0);
     $out = $413;
     HEAP8[$413] = $411;
     $414 = $len;
     $415 = ($414>>>0)>(1);
     if ($415) {
      $416 = $from;
      $417 = (($416) + 1|0);
      $from = $417;
      $418 = HEAP8[$417]|0;
      $419 = $out;
      $420 = (($419) + 1|0);
      $out = $420;
      HEAP8[$420] = $418;
     }
    }
   }
  }
  $459 = $in;
  $460 = $last;
  $461 = ($459>>>0)<($460>>>0);
  if ($461) {
   $462 = $out;
   $463 = $end;
   $464 = ($462>>>0)<($463>>>0);
   $529 = $464;
  } else {
   $529 = 0;
  }
  if (!($529)) {
   break;
  }
 }
 do {
  if ((label|0) == 23) {
   $250 = $0;
   $251 = (($250) + 24|0);
   HEAP32[$251>>2] = 8464;
   $252 = $state;
   HEAP32[$252>>2] = 29;
  }
  else if ((label|0) == 70) {
   $435 = $0;
   $436 = (($435) + 24|0);
   HEAP32[$436>>2] = 8496;
   $437 = $state;
   HEAP32[$437>>2] = 29;
  }
  else if ((label|0) == 74) {
   $452 = $op;
   $453 = $452 & 32;
   $454 = ($453|0)!=(0);
   if ($454) {
    $455 = $state;
    HEAP32[$455>>2] = 11;
    break;
   } else {
    $456 = $0;
    $457 = (($456) + 24|0);
    HEAP32[$457>>2] = 8520;
    $458 = $state;
    HEAP32[$458>>2] = 29;
    break;
   }
  }
 } while(0);
 $465 = $bits;
 $466 = $465 >>> 3;
 $len = $466;
 $467 = $len;
 $468 = $in;
 $469 = (0 - ($467))|0;
 $470 = (($468) + ($469)|0);
 $in = $470;
 $471 = $len;
 $472 = $471 << 3;
 $473 = $bits;
 $474 = (($473) - ($472))|0;
 $bits = $474;
 $475 = $bits;
 $476 = 1 << $475;
 $477 = (($476) - 1)|0;
 $478 = $hold;
 $479 = $478 & $477;
 $hold = $479;
 $480 = $in;
 $481 = (($480) + 1|0);
 $482 = $0;
 HEAP32[$482>>2] = $481;
 $483 = $out;
 $484 = (($483) + 1|0);
 $485 = $0;
 $486 = (($485) + 12|0);
 HEAP32[$486>>2] = $484;
 $487 = $in;
 $488 = $last;
 $489 = ($487>>>0)<($488>>>0);
 if ($489) {
  $490 = $last;
  $491 = $in;
  $492 = $490;
  $493 = $491;
  $494 = (($492) - ($493))|0;
  $495 = (5 + ($494))|0;
  $504 = $495;
 } else {
  $496 = $in;
  $497 = $last;
  $498 = $496;
  $499 = $497;
  $500 = (($498) - ($499))|0;
  $501 = (5 - ($500))|0;
  $504 = $501;
 }
 $502 = $0;
 $503 = (($502) + 4|0);
 HEAP32[$503>>2] = $504;
 $505 = $out;
 $506 = $end;
 $507 = ($505>>>0)<($506>>>0);
 if ($507) {
  $508 = $end;
  $509 = $out;
  $510 = $508;
  $511 = $509;
  $512 = (($510) - ($511))|0;
  $513 = (257 + ($512))|0;
  $522 = $513;
  $520 = $0;
  $521 = (($520) + 16|0);
  HEAP32[$521>>2] = $522;
  $523 = $hold;
  $524 = $state;
  $525 = (($524) + 56|0);
  HEAP32[$525>>2] = $523;
  $526 = $bits;
  $527 = $state;
  $528 = (($527) + 60|0);
  HEAP32[$528>>2] = $526;
  STACKTOP = sp;return;
 } else {
  $514 = $out;
  $515 = $end;
  $516 = $514;
  $517 = $515;
  $518 = (($516) - ($517))|0;
  $519 = (257 - ($518))|0;
  $522 = $519;
  $520 = $0;
  $521 = (($520) + 16|0);
  HEAP32[$521>>2] = $522;
  $523 = $hold;
  $524 = $state;
  $525 = (($524) + 56|0);
  HEAP32[$525>>2] = $523;
  $526 = $bits;
  $527 = $state;
  $528 = (($527) + 60|0);
  HEAP32[$528>>2] = $526;
  STACKTOP = sp;return;
 }
}
function _inflateResetKeep($strm) {
 $strm = $strm|0;
 var $0 = 0, $1 = 0, $10 = 0, $11 = 0, $12 = 0, $13 = 0, $14 = 0, $15 = 0, $16 = 0, $17 = 0, $18 = 0, $19 = 0, $2 = 0, $20 = 0, $21 = 0, $22 = 0, $23 = 0, $24 = 0, $25 = 0, $26 = 0;
 var $27 = 0, $28 = 0, $29 = 0, $3 = 0, $30 = 0, $31 = 0, $32 = 0, $33 = 0, $34 = 0, $35 = 0, $36 = 0, $37 = 0, $38 = 0, $39 = 0, $4 = 0, $40 = 0, $41 = 0, $42 = 0, $43 = 0, $44 = 0;
 var $45 = 0, $46 = 0, $47 = 0, $48 = 0, $49 = 0, $5 = 0, $50 = 0, $51 = 0, $52 = 0, $53 = 0, $54 = 0, $6 = 0, $7 = 0, $8 = 0, $9 = 0, $state = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 16|0;
 $1 = $strm;
 $2 = $1;
 $3 = ($2|0)==(0|0);
 if (!($3)) {
  $4 = $1;
  $5 = (($4) + 28|0);
  $6 = HEAP32[$5>>2]|0;
  $7 = ($6|0)==(0|0);
  if (!($7)) {
   $8 = $1;
   $9 = (($8) + 28|0);
   $10 = HEAP32[$9>>2]|0;
   $state = $10;
   $11 = $state;
   $12 = (($11) + 28|0);
   HEAP32[$12>>2] = 0;
   $13 = $1;
   $14 = (($13) + 20|0);
   HEAP32[$14>>2] = 0;
   $15 = $1;
   $16 = (($15) + 8|0);
   HEAP32[$16>>2] = 0;
   $17 = $1;
   $18 = (($17) + 24|0);
   HEAP32[$18>>2] = 0;
   $19 = $state;
   $20 = (($19) + 8|0);
   $21 = HEAP32[$20>>2]|0;
   $22 = ($21|0)!=(0);
   if ($22) {
    $23 = $state;
    $24 = (($23) + 8|0);
    $25 = HEAP32[$24>>2]|0;
    $26 = $25 & 1;
    $27 = $1;
    $28 = (($27) + 48|0);
    HEAP32[$28>>2] = $26;
   }
   $29 = $state;
   HEAP32[$29>>2] = 0;
   $30 = $state;
   $31 = (($30) + 4|0);
   HEAP32[$31>>2] = 0;
   $32 = $state;
   $33 = (($32) + 12|0);
   HEAP32[$33>>2] = 0;
   $34 = $state;
   $35 = (($34) + 20|0);
   HEAP32[$35>>2] = 32768;
   $36 = $state;
   $37 = (($36) + 32|0);
   HEAP32[$37>>2] = 0;
   $38 = $state;
   $39 = (($38) + 56|0);
   HEAP32[$39>>2] = 0;
   $40 = $state;
   $41 = (($40) + 60|0);
   HEAP32[$41>>2] = 0;
   $42 = $state;
   $43 = (($42) + 1328|0);
   $44 = $state;
   $45 = (($44) + 108|0);
   HEAP32[$45>>2] = $43;
   $46 = $state;
   $47 = (($46) + 80|0);
   HEAP32[$47>>2] = $43;
   $48 = $state;
   $49 = (($48) + 76|0);
   HEAP32[$49>>2] = $43;
   $50 = $state;
   $51 = (($50) + 7104|0);
   HEAP32[$51>>2] = 1;
   $52 = $state;
   $53 = (($52) + 7108|0);
   HEAP32[$53>>2] = -1;
   $0 = 0;
   $54 = $0;
   STACKTOP = sp;return ($54|0);
  }
 }
 $0 = -2;
 $54 = $0;
 STACKTOP = sp;return ($54|0);
}
function _inflateReset($strm) {
 $strm = $strm|0;
 var $0 = 0, $1 = 0, $10 = 0, $11 = 0, $12 = 0, $13 = 0, $14 = 0, $15 = 0, $16 = 0, $17 = 0, $18 = 0, $19 = 0, $2 = 0, $3 = 0, $4 = 0, $5 = 0, $6 = 0, $7 = 0, $8 = 0, $9 = 0;
 var $state = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 16|0;
 $1 = $strm;
 $2 = $1;
 $3 = ($2|0)==(0|0);
 if (!($3)) {
  $4 = $1;
  $5 = (($4) + 28|0);
  $6 = HEAP32[$5>>2]|0;
  $7 = ($6|0)==(0|0);
  if (!($7)) {
   $8 = $1;
   $9 = (($8) + 28|0);
   $10 = HEAP32[$9>>2]|0;
   $state = $10;
   $11 = $state;
   $12 = (($11) + 40|0);
   HEAP32[$12>>2] = 0;
   $13 = $state;
   $14 = (($13) + 44|0);
   HEAP32[$14>>2] = 0;
   $15 = $state;
   $16 = (($15) + 48|0);
   HEAP32[$16>>2] = 0;
   $17 = $1;
   $18 = (_inflateResetKeep($17)|0);
   $0 = $18;
   $19 = $0;
   STACKTOP = sp;return ($19|0);
  }
 }
 $0 = -2;
 $19 = $0;
 STACKTOP = sp;return ($19|0);
}
function _inflateReset2($strm,$windowBits) {
 $strm = $strm|0;
 $windowBits = $windowBits|0;
 var $0 = 0, $1 = 0, $10 = 0, $11 = 0, $12 = 0, $13 = 0, $14 = 0, $15 = 0, $16 = 0, $17 = 0, $18 = 0, $19 = 0, $2 = 0, $20 = 0, $21 = 0, $22 = 0, $23 = 0, $24 = 0, $25 = 0, $26 = 0;
 var $27 = 0, $28 = 0, $29 = 0, $3 = 0, $30 = 0, $31 = 0, $32 = 0, $33 = 0, $34 = 0, $35 = 0, $36 = 0, $37 = 0, $38 = 0, $39 = 0, $4 = 0, $40 = 0, $41 = 0, $42 = 0, $43 = 0, $44 = 0;
 var $45 = 0, $46 = 0, $47 = 0, $48 = 0, $49 = 0, $5 = 0, $50 = 0, $51 = 0, $52 = 0, $53 = 0, $54 = 0, $55 = 0, $56 = 0, $57 = 0, $6 = 0, $7 = 0, $8 = 0, $9 = 0, $state = 0, $wrap = 0;
 var label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 32|0;
 $1 = $strm;
 $2 = $windowBits;
 $3 = $1;
 $4 = ($3|0)==(0|0);
 if (!($4)) {
  $5 = $1;
  $6 = (($5) + 28|0);
  $7 = HEAP32[$6>>2]|0;
  $8 = ($7|0)==(0|0);
  if (!($8)) {
   $9 = $1;
   $10 = (($9) + 28|0);
   $11 = HEAP32[$10>>2]|0;
   $state = $11;
   $12 = $2;
   $13 = ($12|0)<(0);
   if ($13) {
    $wrap = 0;
    $14 = $2;
    $15 = (0 - ($14))|0;
    $2 = $15;
   } else {
    $16 = $2;
    $17 = $16 >> 4;
    $18 = (($17) + 1)|0;
    $wrap = $18;
    $19 = $2;
    $20 = ($19|0)<(48);
    if ($20) {
     $21 = $2;
     $22 = $21 & 15;
     $2 = $22;
    }
   }
   $23 = $2;
   $24 = ($23|0)!=(0);
   do {
    if ($24) {
     $25 = $2;
     $26 = ($25|0)<(8);
     if (!($26)) {
      $27 = $2;
      $28 = ($27|0)>(15);
      if (!($28)) {
       break;
      }
     }
     $0 = -2;
     $57 = $0;
     STACKTOP = sp;return ($57|0);
    }
   } while(0);
   $29 = $state;
   $30 = (($29) + 52|0);
   $31 = HEAP32[$30>>2]|0;
   $32 = ($31|0)!=(0|0);
   if ($32) {
    $33 = $state;
    $34 = (($33) + 36|0);
    $35 = HEAP32[$34>>2]|0;
    $36 = $2;
    $37 = ($35|0)!=($36|0);
    if ($37) {
     $38 = $1;
     $39 = (($38) + 36|0);
     $40 = HEAP32[$39>>2]|0;
     $41 = $1;
     $42 = (($41) + 40|0);
     $43 = HEAP32[$42>>2]|0;
     $44 = $state;
     $45 = (($44) + 52|0);
     $46 = HEAP32[$45>>2]|0;
     FUNCTION_TABLE_vii[$40 & 1]($43,$46);
     $47 = $state;
     $48 = (($47) + 52|0);
     HEAP32[$48>>2] = 0;
    }
   }
   $49 = $wrap;
   $50 = $state;
   $51 = (($50) + 8|0);
   HEAP32[$51>>2] = $49;
   $52 = $2;
   $53 = $state;
   $54 = (($53) + 36|0);
   HEAP32[$54>>2] = $52;
   $55 = $1;
   $56 = (_inflateReset($55)|0);
   $0 = $56;
   $57 = $0;
   STACKTOP = sp;return ($57|0);
  }
 }
 $0 = -2;
 $57 = $0;
 STACKTOP = sp;return ($57|0);
}
function _inflateInit2_($strm,$windowBits,$version,$stream_size) {
 $strm = $strm|0;
 $windowBits = $windowBits|0;
 $version = $version|0;
 $stream_size = $stream_size|0;
 var $0 = 0, $1 = 0, $10 = 0, $11 = 0, $12 = 0, $13 = 0, $14 = 0, $15 = 0, $16 = 0, $17 = 0, $18 = 0, $19 = 0, $2 = 0, $20 = 0, $21 = 0, $22 = 0, $23 = 0, $24 = 0, $25 = 0, $26 = 0;
 var $27 = 0, $28 = 0, $29 = 0, $3 = 0, $30 = 0, $31 = 0, $32 = 0, $33 = 0, $34 = 0, $35 = 0, $36 = 0, $37 = 0, $38 = 0, $39 = 0, $4 = 0, $40 = 0, $41 = 0, $42 = 0, $43 = 0, $44 = 0;
 var $45 = 0, $46 = 0, $47 = 0, $48 = 0, $49 = 0, $5 = 0, $50 = 0, $51 = 0, $52 = 0, $53 = 0, $54 = 0, $55 = 0, $56 = 0, $57 = 0, $58 = 0, $59 = 0, $6 = 0, $60 = 0, $61 = 0, $62 = 0;
 var $7 = 0, $8 = 0, $9 = 0, $ret = 0, $state = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 32|0;
 $1 = $strm;
 $2 = $windowBits;
 $3 = $version;
 $4 = $stream_size;
 $5 = $3;
 $6 = ($5|0)==(0|0);
 if (!($6)) {
  $7 = $3;
  $8 = HEAP8[$7]|0;
  $9 = $8 << 24 >> 24;
  $10 = HEAP8[8]|0;
  $11 = $10 << 24 >> 24;
  $12 = ($9|0)!=($11|0);
  if (!($12)) {
   $13 = $4;
   $14 = ($13|0)!=(56);
   if (!($14)) {
    $15 = $1;
    $16 = ($15|0)==(0|0);
    if ($16) {
     $0 = -2;
     $62 = $0;
     STACKTOP = sp;return ($62|0);
    }
    $17 = $1;
    $18 = (($17) + 24|0);
    HEAP32[$18>>2] = 0;
    $19 = $1;
    $20 = (($19) + 32|0);
    $21 = HEAP32[$20>>2]|0;
    $22 = ($21|0)==(0|0);
    if ($22) {
     $23 = $1;
     $24 = (($23) + 32|0);
     HEAP32[$24>>2] = 1;
     $25 = $1;
     $26 = (($25) + 40|0);
     HEAP32[$26>>2] = 0;
    }
    $27 = $1;
    $28 = (($27) + 36|0);
    $29 = HEAP32[$28>>2]|0;
    $30 = ($29|0)==(0|0);
    if ($30) {
     $31 = $1;
     $32 = (($31) + 36|0);
     HEAP32[$32>>2] = 1;
    }
    $33 = $1;
    $34 = (($33) + 32|0);
    $35 = HEAP32[$34>>2]|0;
    $36 = $1;
    $37 = (($36) + 40|0);
    $38 = HEAP32[$37>>2]|0;
    $39 = (FUNCTION_TABLE_iiii[$35 & 1]($38,1,7116)|0);
    $state = $39;
    $40 = $state;
    $41 = ($40|0)==(0|0);
    if ($41) {
     $0 = -4;
     $62 = $0;
     STACKTOP = sp;return ($62|0);
    }
    $42 = $state;
    $43 = $1;
    $44 = (($43) + 28|0);
    HEAP32[$44>>2] = $42;
    $45 = $state;
    $46 = (($45) + 52|0);
    HEAP32[$46>>2] = 0;
    $47 = $1;
    $48 = $2;
    $49 = (_inflateReset2($47,$48)|0);
    $ret = $49;
    $50 = $ret;
    $51 = ($50|0)!=(0);
    if ($51) {
     $52 = $1;
     $53 = (($52) + 36|0);
     $54 = HEAP32[$53>>2]|0;
     $55 = $1;
     $56 = (($55) + 40|0);
     $57 = HEAP32[$56>>2]|0;
     $58 = $state;
     FUNCTION_TABLE_vii[$54 & 1]($57,$58);
     $59 = $1;
     $60 = (($59) + 28|0);
     HEAP32[$60>>2] = 0;
    }
    $61 = $ret;
    $0 = $61;
    $62 = $0;
    STACKTOP = sp;return ($62|0);
   }
  }
 }
 $0 = -6;
 $62 = $0;
 STACKTOP = sp;return ($62|0);
}
function _inflateInit_($strm,$version,$stream_size) {
 $strm = $strm|0;
 $version = $version|0;
 $stream_size = $stream_size|0;
 var $0 = 0, $1 = 0, $2 = 0, $3 = 0, $4 = 0, $5 = 0, $6 = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 16|0;
 $0 = $strm;
 $1 = $version;
 $2 = $stream_size;
 $3 = $0;
 $4 = $1;
 $5 = $2;
 $6 = (_inflateInit2_($3,15,$4,$5)|0);
 STACKTOP = sp;return ($6|0);
}
function _inflate($strm,$flush) {
 $strm = $strm|0;
 $flush = $flush|0;
 var $0 = 0, $1 = 0, $10 = 0, $100 = 0, $1000 = 0, $1001 = 0, $1002 = 0, $1003 = 0, $1004 = 0, $1005 = 0, $1006 = 0, $1007 = 0, $1008 = 0, $1009 = 0, $101 = 0, $1010 = 0, $1011 = 0, $1012 = 0, $1013 = 0, $1014 = 0;
 var $1015 = 0, $1016 = 0, $1017 = 0, $1018 = 0, $1019 = 0, $102 = 0, $1020 = 0, $1021 = 0, $1022 = 0, $1023 = 0, $1024 = 0, $1025 = 0, $1026 = 0, $1027 = 0, $1028 = 0, $1029 = 0, $103 = 0, $1030 = 0, $1031 = 0, $1032 = 0;
 var $1033 = 0, $1034 = 0, $1035 = 0, $1036 = 0, $1037 = 0, $1038 = 0, $1039 = 0, $104 = 0, $1040 = 0, $1041 = 0, $1042 = 0, $1043 = 0, $1044 = 0, $1045 = 0, $1046 = 0, $1047 = 0, $1048 = 0, $1049 = 0, $105 = 0, $1050 = 0;
 var $1051 = 0, $1052 = 0, $1053 = 0, $1054 = 0, $1055 = 0, $1056 = 0, $1057 = 0, $1058 = 0, $1059 = 0, $106 = 0, $1060 = 0, $1061 = 0, $1062 = 0, $1063 = 0, $1064 = 0, $1065 = 0, $1066 = 0, $1067 = 0, $1068 = 0, $1069 = 0;
 var $107 = 0, $1070 = 0, $1071 = 0, $1072 = 0, $1073 = 0, $1074 = 0, $1075 = 0, $1076 = 0, $1077 = 0, $1078 = 0, $1079 = 0, $108 = 0, $1080 = 0, $1081 = 0, $1082 = 0, $1083 = 0, $1084 = 0, $1085 = 0, $1086 = 0, $1087 = 0;
 var $1088 = 0, $1089 = 0, $109 = 0, $1090 = 0, $1091 = 0, $1092 = 0, $1093 = 0, $1094 = 0, $1095 = 0, $1096 = 0, $1097 = 0, $1098 = 0, $1099 = 0, $11 = 0, $110 = 0, $1100 = 0, $1101 = 0, $1102 = 0, $1103 = 0, $1104 = 0;
 var $1105 = 0, $1106 = 0, $1107 = 0, $1108 = 0, $1109 = 0, $111 = 0, $1110 = 0, $1111 = 0, $1112 = 0, $1113 = 0, $1114 = 0, $1115 = 0, $1116 = 0, $1117 = 0, $1118 = 0, $1119 = 0, $112 = 0, $1120 = 0, $1121 = 0, $1122 = 0;
 var $1123 = 0, $1124 = 0, $1125 = 0, $1126 = 0, $1127 = 0, $1128 = 0, $1129 = 0, $113 = 0, $1130 = 0, $1131 = 0, $1132 = 0, $1133 = 0, $1134 = 0, $1135 = 0, $1136 = 0, $1137 = 0, $1138 = 0, $1139 = 0, $114 = 0, $1140 = 0;
 var $1141 = 0, $1142 = 0, $1143 = 0, $1144 = 0, $1145 = 0, $1146 = 0, $1147 = 0, $1148 = 0, $1149 = 0, $115 = 0, $1150 = 0, $1151 = 0, $1152 = 0, $1153 = 0, $1154 = 0, $1155 = 0, $1156 = 0, $1157 = 0, $1158 = 0, $1159 = 0;
 var $116 = 0, $1160 = 0, $1161 = 0, $1162 = 0, $1163 = 0, $1164 = 0, $1165 = 0, $1166 = 0, $1167 = 0, $1168 = 0, $1169 = 0, $117 = 0, $1170 = 0, $1171 = 0, $1172 = 0, $1173 = 0, $1174 = 0, $1175 = 0, $1176 = 0, $1177 = 0;
 var $1178 = 0, $1179 = 0, $118 = 0, $1180 = 0, $1181 = 0, $1182 = 0, $1183 = 0, $1184 = 0, $1185 = 0, $1186 = 0, $1187 = 0, $1188 = 0, $1189 = 0, $119 = 0, $1190 = 0, $1191 = 0, $1192 = 0, $1193 = 0, $1194 = 0, $1195 = 0;
 var $1196 = 0, $1197 = 0, $1198 = 0, $1199 = 0, $12 = 0, $120 = 0, $1200 = 0, $1201 = 0, $1202 = 0, $1203 = 0, $1204 = 0, $1205 = 0, $1206 = 0, $1207 = 0, $1208 = 0, $1209 = 0, $121 = 0, $1210 = 0, $1211 = 0, $1212 = 0;
 var $1213 = 0, $1214 = 0, $1215 = 0, $1216 = 0, $1217 = 0, $1218 = 0, $1219 = 0, $122 = 0, $1220 = 0, $1221 = 0, $1222 = 0, $1223 = 0, $1224 = 0, $1225 = 0, $1226 = 0, $1227 = 0, $1228 = 0, $1229 = 0, $123 = 0, $1230 = 0;
 var $1231 = 0, $1232 = 0, $1233 = 0, $1234 = 0, $1235 = 0, $1236 = 0, $1237 = 0, $1238 = 0, $1239 = 0, $124 = 0, $1240 = 0, $1241 = 0, $1242 = 0, $1243 = 0, $1244 = 0, $1245 = 0, $1246 = 0, $1247 = 0, $1248 = 0, $1249 = 0;
 var $125 = 0, $1250 = 0, $1251 = 0, $1252 = 0, $1253 = 0, $1254 = 0, $1255 = 0, $1256 = 0, $1257 = 0, $1258 = 0, $1259 = 0, $126 = 0, $1260 = 0, $1261 = 0, $1262 = 0, $1263 = 0, $1264 = 0, $1265 = 0, $1266 = 0, $1267 = 0;
 var $1268 = 0, $1269 = 0, $127 = 0, $1270 = 0, $1271 = 0, $1272 = 0, $1273 = 0, $1274 = 0, $1275 = 0, $1276 = 0, $1277 = 0, $1278 = 0, $1279 = 0, $128 = 0, $1280 = 0, $1281 = 0, $1282 = 0, $1283 = 0, $1284 = 0, $1285 = 0;
 var $1286 = 0, $1287 = 0, $1288 = 0, $1289 = 0, $129 = 0, $1290 = 0, $1291 = 0, $1292 = 0, $1293 = 0, $1294 = 0, $1295 = 0, $1296 = 0, $1297 = 0, $1298 = 0, $1299 = 0, $13 = 0, $130 = 0, $1300 = 0, $1301 = 0, $1302 = 0;
 var $1303 = 0, $1304 = 0, $1305 = 0, $1306 = 0, $1307 = 0, $1308 = 0, $1309 = 0, $131 = 0, $1310 = 0, $1311 = 0, $1312 = 0, $1313 = 0, $1314 = 0, $1315 = 0, $1316 = 0, $1317 = 0, $1318 = 0, $1319 = 0, $132 = 0, $1320 = 0;
 var $1321 = 0, $1322 = 0, $1323 = 0, $1324 = 0, $1325 = 0, $1326 = 0, $1327 = 0, $1328 = 0, $1329 = 0, $133 = 0, $1330 = 0, $1331 = 0, $1332 = 0, $1333 = 0, $1334 = 0, $1335 = 0, $1336 = 0, $1337 = 0, $1338 = 0, $1339 = 0;
 var $134 = 0, $1340 = 0, $1341 = 0, $1342 = 0, $1343 = 0, $1344 = 0, $1345 = 0, $1346 = 0, $1347 = 0, $1348 = 0, $1349 = 0, $135 = 0, $1350 = 0, $1351 = 0, $1352 = 0, $1353 = 0, $1354 = 0, $1355 = 0, $1356 = 0, $1357 = 0;
 var $1358 = 0, $1359 = 0, $136 = 0, $1360 = 0, $1361 = 0, $1362 = 0, $1363 = 0, $1364 = 0, $1365 = 0, $1366 = 0, $1367 = 0, $1368 = 0, $1369 = 0, $137 = 0, $1370 = 0, $1371 = 0, $1372 = 0, $1373 = 0, $1374 = 0, $1375 = 0;
 var $1376 = 0, $1377 = 0, $1378 = 0, $1379 = 0, $138 = 0, $1380 = 0, $1381 = 0, $1382 = 0, $1383 = 0, $1384 = 0, $1385 = 0, $1386 = 0, $1387 = 0, $1388 = 0, $1389 = 0, $139 = 0, $1390 = 0, $1391 = 0, $1392 = 0, $1393 = 0;
 var $1394 = 0, $1395 = 0, $1396 = 0, $1397 = 0, $1398 = 0, $1399 = 0, $14 = 0, $140 = 0, $1400 = 0, $1401 = 0, $1402 = 0, $1403 = 0, $1404 = 0, $1405 = 0, $1406 = 0, $1407 = 0, $1408 = 0, $1409 = 0, $141 = 0, $1410 = 0;
 var $1411 = 0, $1412 = 0, $1413 = 0, $1414 = 0, $1415 = 0, $1416 = 0, $1417 = 0, $1418 = 0, $1419 = 0, $142 = 0, $1420 = 0, $1421 = 0, $1422 = 0, $1423 = 0, $1424 = 0, $1425 = 0, $1426 = 0, $1427 = 0, $1428 = 0, $1429 = 0;
 var $143 = 0, $1430 = 0, $1431 = 0, $1432 = 0, $1433 = 0, $1434 = 0, $1435 = 0, $1436 = 0, $1437 = 0, $1438 = 0, $1439 = 0, $144 = 0, $1440 = 0, $1441 = 0, $1442 = 0, $1443 = 0, $1444 = 0, $1445 = 0, $1446 = 0, $1447 = 0;
 var $1448 = 0, $1449 = 0, $145 = 0, $1450 = 0, $1451 = 0, $1452 = 0, $1453 = 0, $1454 = 0, $1455 = 0, $1456 = 0, $1457 = 0, $1458 = 0, $1459 = 0, $146 = 0, $1460 = 0, $1461 = 0, $1462 = 0, $1463 = 0, $1464 = 0, $1465 = 0;
 var $1466 = 0, $1467 = 0, $1468 = 0, $1469 = 0, $147 = 0, $1470 = 0, $1471 = 0, $1472 = 0, $1473 = 0, $1474 = 0, $1475 = 0, $1476 = 0, $1477 = 0, $1478 = 0, $1479 = 0, $148 = 0, $1480 = 0, $1481 = 0, $1482 = 0, $1483 = 0;
 var $1484 = 0, $1485 = 0, $1486 = 0, $1487 = 0, $1488 = 0, $1489 = 0, $149 = 0, $1490 = 0, $1491 = 0, $1492 = 0, $1493 = 0, $1494 = 0, $1495 = 0, $1496 = 0, $1497 = 0, $1498 = 0, $1499 = 0, $15 = 0, $150 = 0, $1500 = 0;
 var $1501 = 0, $1502 = 0, $1503 = 0, $1504 = 0, $1505 = 0, $1506 = 0, $1507 = 0, $1508 = 0, $1509 = 0, $151 = 0, $1510 = 0, $1511 = 0, $1512 = 0, $1513 = 0, $1514 = 0, $1515 = 0, $1516 = 0, $1517 = 0, $1518 = 0, $1519 = 0;
 var $152 = 0, $1520 = 0, $1521 = 0, $1522 = 0, $1523 = 0, $1524 = 0, $1525 = 0, $1526 = 0, $1527 = 0, $1528 = 0, $1529 = 0, $153 = 0, $1530 = 0, $1531 = 0, $1532 = 0, $1533 = 0, $1534 = 0, $1535 = 0, $1536 = 0, $1537 = 0;
 var $1538 = 0, $1539 = 0, $154 = 0, $1540 = 0, $1541 = 0, $1542 = 0, $1543 = 0, $1544 = 0, $1545 = 0, $1546 = 0, $1547 = 0, $1548 = 0, $1549 = 0, $155 = 0, $1550 = 0, $1551 = 0, $1552 = 0, $1553 = 0, $1554 = 0, $1555 = 0;
 var $1556 = 0, $1557 = 0, $1558 = 0, $1559 = 0, $156 = 0, $1560 = 0, $1561 = 0, $1562 = 0, $1563 = 0, $1564 = 0, $1565 = 0, $1566 = 0, $1567 = 0, $1568 = 0, $1569 = 0, $157 = 0, $1570 = 0, $1571 = 0, $1572 = 0, $1573 = 0;
 var $1574 = 0, $1575 = 0, $1576 = 0, $1577 = 0, $1578 = 0, $1579 = 0, $158 = 0, $1580 = 0, $1581 = 0, $1582 = 0, $1583 = 0, $1584 = 0, $1585 = 0, $1586 = 0, $1587 = 0, $1588 = 0, $1589 = 0, $159 = 0, $1590 = 0, $1591 = 0;
 var $1592 = 0, $1593 = 0, $1594 = 0, $1595 = 0, $1596 = 0, $1597 = 0, $1598 = 0, $1599 = 0, $16 = 0, $160 = 0, $1600 = 0, $1601 = 0, $1602 = 0, $1603 = 0, $1604 = 0, $1605 = 0, $1606 = 0, $1607 = 0, $1608 = 0, $1609 = 0;
 var $161 = 0, $1610 = 0, $1611 = 0, $1612 = 0, $1613 = 0, $1614 = 0, $1615 = 0, $1616 = 0, $1617 = 0, $1618 = 0, $1619 = 0, $162 = 0, $1620 = 0, $1621 = 0, $1622 = 0, $1623 = 0, $1624 = 0, $1625 = 0, $1626 = 0, $1627 = 0;
 var $1628 = 0, $1629 = 0, $163 = 0, $1630 = 0, $1631 = 0, $1632 = 0, $1633 = 0, $1634 = 0, $1635 = 0, $1636 = 0, $1637 = 0, $1638 = 0, $1639 = 0, $164 = 0, $1640 = 0, $1641 = 0, $1642 = 0, $1643 = 0, $1644 = 0, $1645 = 0;
 var $1646 = 0, $1647 = 0, $1648 = 0, $1649 = 0, $165 = 0, $1650 = 0, $1651 = 0, $1652 = 0, $1653 = 0, $1654 = 0, $1655 = 0, $1656 = 0, $1657 = 0, $1658 = 0, $1659 = 0, $166 = 0, $1660 = 0, $1661 = 0, $1662 = 0, $1663 = 0;
 var $1664 = 0, $1665 = 0, $1666 = 0, $1667 = 0, $1668 = 0, $1669 = 0, $167 = 0, $1670 = 0, $1671 = 0, $1672 = 0, $1673 = 0, $1674 = 0, $1675 = 0, $1676 = 0, $1677 = 0, $1678 = 0, $1679 = 0, $168 = 0, $1680 = 0, $1681 = 0;
 var $1682 = 0, $1683 = 0, $1684 = 0, $1685 = 0, $1686 = 0, $1687 = 0, $1688 = 0, $1689 = 0, $169 = 0, $1690 = 0, $1691 = 0, $1692 = 0, $1693 = 0, $1694 = 0, $1695 = 0, $1696 = 0, $1697 = 0, $1698 = 0, $1699 = 0, $17 = 0;
 var $170 = 0, $1700 = 0, $1701 = 0, $1702 = 0, $1703 = 0, $1704 = 0, $1705 = 0, $1706 = 0, $1707 = 0, $1708 = 0, $1709 = 0, $171 = 0, $1710 = 0, $1711 = 0, $1712 = 0, $1713 = 0, $1714 = 0, $1715 = 0, $1716 = 0, $1717 = 0;
 var $1718 = 0, $1719 = 0, $172 = 0, $1720 = 0, $1721 = 0, $1722 = 0, $1723 = 0, $1724 = 0, $1725 = 0, $1726 = 0, $1727 = 0, $1728 = 0, $1729 = 0, $173 = 0, $1730 = 0, $1731 = 0, $1732 = 0, $1733 = 0, $1734 = 0, $1735 = 0;
 var $1736 = 0, $1737 = 0, $1738 = 0, $1739 = 0, $174 = 0, $1740 = 0, $1741 = 0, $1742 = 0, $1743 = 0, $1744 = 0, $1745 = 0, $1746 = 0, $1747 = 0, $1748 = 0, $1749 = 0, $175 = 0, $1750 = 0, $1751 = 0, $1752 = 0, $1753 = 0;
 var $1754 = 0, $1755 = 0, $1756 = 0, $1757 = 0, $1758 = 0, $1759 = 0, $176 = 0, $1760 = 0, $1761 = 0, $1762 = 0, $1763 = 0, $1764 = 0, $1765 = 0, $1766 = 0, $1767 = 0, $1768 = 0, $1769 = 0, $177 = 0, $1770 = 0, $1771 = 0;
 var $1772 = 0, $1773 = 0, $1774 = 0, $1775 = 0, $1776 = 0, $1777 = 0, $1778 = 0, $1779 = 0, $178 = 0, $1780 = 0, $1781 = 0, $1782 = 0, $1783 = 0, $1784 = 0, $1785 = 0, $1786 = 0, $1787 = 0, $1788 = 0, $1789 = 0, $179 = 0;
 var $1790 = 0, $1791 = 0, $1792 = 0, $1793 = 0, $1794 = 0, $1795 = 0, $1796 = 0, $1797 = 0, $1798 = 0, $1799 = 0, $18 = 0, $180 = 0, $1800 = 0, $1801 = 0, $1802 = 0, $1803 = 0, $1804 = 0, $1805 = 0, $1806 = 0, $1807 = 0;
 var $1808 = 0, $1809 = 0, $181 = 0, $1810 = 0, $1811 = 0, $1812 = 0, $1813 = 0, $1814 = 0, $1815 = 0, $1816 = 0, $1817 = 0, $1818 = 0, $1819 = 0, $182 = 0, $1820 = 0, $1821 = 0, $1822 = 0, $1823 = 0, $1824 = 0, $1825 = 0;
 var $1826 = 0, $1827 = 0, $1828 = 0, $1829 = 0, $183 = 0, $1830 = 0, $1831 = 0, $1832 = 0, $1833 = 0, $1834 = 0, $1835 = 0, $1836 = 0, $1837 = 0, $1838 = 0, $1839 = 0, $184 = 0, $1840 = 0, $1841 = 0, $1842 = 0, $1843 = 0;
 var $1844 = 0, $1845 = 0, $1846 = 0, $1847 = 0, $1848 = 0, $1849 = 0, $185 = 0, $1850 = 0, $1851 = 0, $1852 = 0, $1853 = 0, $1854 = 0, $1855 = 0, $1856 = 0, $1857 = 0, $1858 = 0, $1859 = 0, $186 = 0, $1860 = 0, $1861 = 0;
 var $1862 = 0, $1863 = 0, $1864 = 0, $1865 = 0, $1866 = 0, $1867 = 0, $1868 = 0, $1869 = 0, $187 = 0, $1870 = 0, $1871 = 0, $1872 = 0, $1873 = 0, $1874 = 0, $1875 = 0, $1876 = 0, $1877 = 0, $1878 = 0, $1879 = 0, $188 = 0;
 var $1880 = 0, $1881 = 0, $1882 = 0, $1883 = 0, $1884 = 0, $1885 = 0, $1886 = 0, $1887 = 0, $1888 = 0, $1889 = 0, $189 = 0, $1890 = 0, $1891 = 0, $1892 = 0, $1893 = 0, $1894 = 0, $1895 = 0, $1896 = 0, $1897 = 0, $1898 = 0;
 var $1899 = 0, $19 = 0, $190 = 0, $1900 = 0, $1901 = 0, $1902 = 0, $1903 = 0, $1904 = 0, $1905 = 0, $1906 = 0, $1907 = 0, $1908 = 0, $1909 = 0, $191 = 0, $1910 = 0, $1911 = 0, $1912 = 0, $1913 = 0, $1914 = 0, $1915 = 0;
 var $1916 = 0, $1917 = 0, $1918 = 0, $1919 = 0, $192 = 0, $1920 = 0, $1921 = 0, $1922 = 0, $1923 = 0, $1924 = 0, $1925 = 0, $1926 = 0, $1927 = 0, $1928 = 0, $1929 = 0, $193 = 0, $1930 = 0, $1931 = 0, $1932 = 0, $1933 = 0;
 var $1934 = 0, $1935 = 0, $1936 = 0, $1937 = 0, $1938 = 0, $1939 = 0, $194 = 0, $1940 = 0, $1941 = 0, $1942 = 0, $1943 = 0, $1944 = 0, $1945 = 0, $1946 = 0, $1947 = 0, $1948 = 0, $1949 = 0, $195 = 0, $1950 = 0, $1951 = 0;
 var $1952 = 0, $1953 = 0, $1954 = 0, $1955 = 0, $1956 = 0, $1957 = 0, $1958 = 0, $1959 = 0, $196 = 0, $1960 = 0, $1961 = 0, $1962 = 0, $1963 = 0, $1964 = 0, $1965 = 0, $1966 = 0, $1967 = 0, $1968 = 0, $1969 = 0, $197 = 0;
 var $1970 = 0, $1971 = 0, $1972 = 0, $1973 = 0, $1974 = 0, $1975 = 0, $1976 = 0, $1977 = 0, $1978 = 0, $1979 = 0, $198 = 0, $1980 = 0, $1981 = 0, $1982 = 0, $1983 = 0, $1984 = 0, $1985 = 0, $1986 = 0, $1987 = 0, $1988 = 0;
 var $1989 = 0, $199 = 0, $1990 = 0, $1991 = 0, $1992 = 0, $1993 = 0, $1994 = 0, $1995 = 0, $1996 = 0, $1997 = 0, $1998 = 0, $1999 = 0, $2 = 0, $20 = 0, $200 = 0, $2000 = 0, $2001 = 0, $2002 = 0, $2003 = 0, $2004 = 0;
 var $2005 = 0, $2006 = 0, $2007 = 0, $2008 = 0, $2009 = 0, $201 = 0, $2010 = 0, $2011 = 0, $2012 = 0, $2013 = 0, $2014 = 0, $2015 = 0, $2016 = 0, $2017 = 0, $2018 = 0, $2019 = 0, $202 = 0, $2020 = 0, $2021 = 0, $2022 = 0;
 var $2023 = 0, $2024 = 0, $2025 = 0, $2026 = 0, $2027 = 0, $2028 = 0, $2029 = 0, $203 = 0, $2030 = 0, $2031 = 0, $2032 = 0, $2033 = 0, $2034 = 0, $2035 = 0, $2036 = 0, $2037 = 0, $2038 = 0, $2039 = 0, $204 = 0, $2040 = 0;
 var $2041 = 0, $2042 = 0, $2043 = 0, $2044 = 0, $2045 = 0, $2046 = 0, $2047 = 0, $2048 = 0, $2049 = 0, $205 = 0, $2050 = 0, $2051 = 0, $2052 = 0, $2053 = 0, $2054 = 0, $2055 = 0, $2056 = 0, $2057 = 0, $2058 = 0, $2059 = 0;
 var $206 = 0, $2060 = 0, $2061 = 0, $2062 = 0, $2063 = 0, $2064 = 0, $2065 = 0, $2066 = 0, $2067 = 0, $2068 = 0, $2069 = 0, $207 = 0, $2070 = 0, $2071 = 0, $2072 = 0, $2073 = 0, $2074 = 0, $2075 = 0, $2076 = 0, $2077 = 0;
 var $2078 = 0, $2079 = 0, $208 = 0, $2080 = 0, $2081 = 0, $2082 = 0, $2083 = 0, $2084 = 0, $2085 = 0, $2086 = 0, $2087 = 0, $2088 = 0, $2089 = 0, $209 = 0, $2090 = 0, $2091 = 0, $2092 = 0, $2093 = 0, $2094 = 0, $2095 = 0;
 var $2096 = 0, $2097 = 0, $2098 = 0, $2099 = 0, $21 = 0, $210 = 0, $2100 = 0, $2101 = 0, $2102 = 0, $2103 = 0, $2104 = 0, $2105 = 0, $2106 = 0, $2107 = 0, $2108 = 0, $2109 = 0, $211 = 0, $2110 = 0, $2111 = 0, $2112 = 0;
 var $2113 = 0, $2114 = 0, $2115 = 0, $2116 = 0, $2117 = 0, $2118 = 0, $2119 = 0, $212 = 0, $2120 = 0, $2121 = 0, $2122 = 0, $2123 = 0, $2124 = 0, $2125 = 0, $2126 = 0, $2127 = 0, $2128 = 0, $2129 = 0, $213 = 0, $2130 = 0;
 var $2131 = 0, $2132 = 0, $2133 = 0, $214 = 0, $215 = 0, $216 = 0, $217 = 0, $218 = 0, $219 = 0, $22 = 0, $220 = 0, $221 = 0, $222 = 0, $223 = 0, $224 = 0, $225 = 0, $226 = 0, $227 = 0, $228 = 0, $229 = 0;
 var $23 = 0, $230 = 0, $231 = 0, $232 = 0, $233 = 0, $234 = 0, $235 = 0, $236 = 0, $237 = 0, $238 = 0, $239 = 0, $24 = 0, $240 = 0, $241 = 0, $242 = 0, $243 = 0, $244 = 0, $245 = 0, $246 = 0, $247 = 0;
 var $248 = 0, $249 = 0, $25 = 0, $250 = 0, $251 = 0, $252 = 0, $253 = 0, $254 = 0, $255 = 0, $256 = 0, $257 = 0, $258 = 0, $259 = 0, $26 = 0, $260 = 0, $261 = 0, $262 = 0, $263 = 0, $264 = 0, $265 = 0;
 var $266 = 0, $267 = 0, $268 = 0, $269 = 0, $27 = 0, $270 = 0, $271 = 0, $272 = 0, $273 = 0, $274 = 0, $275 = 0, $276 = 0, $277 = 0, $278 = 0, $279 = 0, $28 = 0, $280 = 0, $281 = 0, $282 = 0, $283 = 0;
 var $284 = 0, $285 = 0, $286 = 0, $287 = 0, $288 = 0, $289 = 0, $29 = 0, $290 = 0, $291 = 0, $292 = 0, $293 = 0, $294 = 0, $295 = 0, $296 = 0, $297 = 0, $298 = 0, $299 = 0, $3 = 0, $30 = 0, $300 = 0;
 var $301 = 0, $302 = 0, $303 = 0, $304 = 0, $305 = 0, $306 = 0, $307 = 0, $308 = 0, $309 = 0, $31 = 0, $310 = 0, $311 = 0, $312 = 0, $313 = 0, $314 = 0, $315 = 0, $316 = 0, $317 = 0, $318 = 0, $319 = 0;
 var $32 = 0, $320 = 0, $321 = 0, $322 = 0, $323 = 0, $324 = 0, $325 = 0, $326 = 0, $327 = 0, $328 = 0, $329 = 0, $33 = 0, $330 = 0, $331 = 0, $332 = 0, $333 = 0, $334 = 0, $335 = 0, $336 = 0, $337 = 0;
 var $338 = 0, $339 = 0, $34 = 0, $340 = 0, $341 = 0, $342 = 0, $343 = 0, $344 = 0, $345 = 0, $346 = 0, $347 = 0, $348 = 0, $349 = 0, $35 = 0, $350 = 0, $351 = 0, $352 = 0, $353 = 0, $354 = 0, $355 = 0;
 var $356 = 0, $357 = 0, $358 = 0, $359 = 0, $36 = 0, $360 = 0, $361 = 0, $362 = 0, $363 = 0, $364 = 0, $365 = 0, $366 = 0, $367 = 0, $368 = 0, $369 = 0, $37 = 0, $370 = 0, $371 = 0, $372 = 0, $373 = 0;
 var $374 = 0, $375 = 0, $376 = 0, $377 = 0, $378 = 0, $379 = 0, $38 = 0, $380 = 0, $381 = 0, $382 = 0, $383 = 0, $384 = 0, $385 = 0, $386 = 0, $387 = 0, $388 = 0, $389 = 0, $39 = 0, $390 = 0, $391 = 0;
 var $392 = 0, $393 = 0, $394 = 0, $395 = 0, $396 = 0, $397 = 0, $398 = 0, $399 = 0, $4 = 0, $40 = 0, $400 = 0, $401 = 0, $402 = 0, $403 = 0, $404 = 0, $405 = 0, $406 = 0, $407 = 0, $408 = 0, $409 = 0;
 var $41 = 0, $410 = 0, $411 = 0, $412 = 0, $413 = 0, $414 = 0, $415 = 0, $416 = 0, $417 = 0, $418 = 0, $419 = 0, $42 = 0, $420 = 0, $421 = 0, $422 = 0, $423 = 0, $424 = 0, $425 = 0, $426 = 0, $427 = 0;
 var $428 = 0, $429 = 0, $43 = 0, $430 = 0, $431 = 0, $432 = 0, $433 = 0, $434 = 0, $435 = 0, $436 = 0, $437 = 0, $438 = 0, $439 = 0, $44 = 0, $440 = 0, $441 = 0, $442 = 0, $443 = 0, $444 = 0, $445 = 0;
 var $446 = 0, $447 = 0, $448 = 0, $449 = 0, $45 = 0, $450 = 0, $451 = 0, $452 = 0, $453 = 0, $454 = 0, $455 = 0, $456 = 0, $457 = 0, $458 = 0, $459 = 0, $46 = 0, $460 = 0, $461 = 0, $462 = 0, $463 = 0;
 var $464 = 0, $465 = 0, $466 = 0, $467 = 0, $468 = 0, $469 = 0, $47 = 0, $470 = 0, $471 = 0, $472 = 0, $473 = 0, $474 = 0, $475 = 0, $476 = 0, $477 = 0, $478 = 0, $479 = 0, $48 = 0, $480 = 0, $481 = 0;
 var $482 = 0, $483 = 0, $484 = 0, $485 = 0, $486 = 0, $487 = 0, $488 = 0, $489 = 0, $49 = 0, $490 = 0, $491 = 0, $492 = 0, $493 = 0, $494 = 0, $495 = 0, $496 = 0, $497 = 0, $498 = 0, $499 = 0, $5 = 0;
 var $50 = 0, $500 = 0, $501 = 0, $502 = 0, $503 = 0, $504 = 0, $505 = 0, $506 = 0, $507 = 0, $508 = 0, $509 = 0, $51 = 0, $510 = 0, $511 = 0, $512 = 0, $513 = 0, $514 = 0, $515 = 0, $516 = 0, $517 = 0;
 var $518 = 0, $519 = 0, $52 = 0, $520 = 0, $521 = 0, $522 = 0, $523 = 0, $524 = 0, $525 = 0, $526 = 0, $527 = 0, $528 = 0, $529 = 0, $53 = 0, $530 = 0, $531 = 0, $532 = 0, $533 = 0, $534 = 0, $535 = 0;
 var $536 = 0, $537 = 0, $538 = 0, $539 = 0, $54 = 0, $540 = 0, $541 = 0, $542 = 0, $543 = 0, $544 = 0, $545 = 0, $546 = 0, $547 = 0, $548 = 0, $549 = 0, $55 = 0, $550 = 0, $551 = 0, $552 = 0, $553 = 0;
 var $554 = 0, $555 = 0, $556 = 0, $557 = 0, $558 = 0, $559 = 0, $56 = 0, $560 = 0, $561 = 0, $562 = 0, $563 = 0, $564 = 0, $565 = 0, $566 = 0, $567 = 0, $568 = 0, $569 = 0, $57 = 0, $570 = 0, $571 = 0;
 var $572 = 0, $573 = 0, $574 = 0, $575 = 0, $576 = 0, $577 = 0, $578 = 0, $579 = 0, $58 = 0, $580 = 0, $581 = 0, $582 = 0, $583 = 0, $584 = 0, $585 = 0, $586 = 0, $587 = 0, $588 = 0, $589 = 0, $59 = 0;
 var $590 = 0, $591 = 0, $592 = 0, $593 = 0, $594 = 0, $595 = 0, $596 = 0, $597 = 0, $598 = 0, $599 = 0, $6 = 0, $60 = 0, $600 = 0, $601 = 0, $602 = 0, $603 = 0, $604 = 0, $605 = 0, $606 = 0, $607 = 0;
 var $608 = 0, $609 = 0, $61 = 0, $610 = 0, $611 = 0, $612 = 0, $613 = 0, $614 = 0, $615 = 0, $616 = 0, $617 = 0, $618 = 0, $619 = 0, $62 = 0, $620 = 0, $621 = 0, $622 = 0, $623 = 0, $624 = 0, $625 = 0;
 var $626 = 0, $627 = 0, $628 = 0, $629 = 0, $63 = 0, $630 = 0, $631 = 0, $632 = 0, $633 = 0, $634 = 0, $635 = 0, $636 = 0, $637 = 0, $638 = 0, $639 = 0, $64 = 0, $640 = 0, $641 = 0, $642 = 0, $643 = 0;
 var $644 = 0, $645 = 0, $646 = 0, $647 = 0, $648 = 0, $649 = 0, $65 = 0, $650 = 0, $651 = 0, $652 = 0, $653 = 0, $654 = 0, $655 = 0, $656 = 0, $657 = 0, $658 = 0, $659 = 0, $66 = 0, $660 = 0, $661 = 0;
 var $662 = 0, $663 = 0, $664 = 0, $665 = 0, $666 = 0, $667 = 0, $668 = 0, $669 = 0, $67 = 0, $670 = 0, $671 = 0, $672 = 0, $673 = 0, $674 = 0, $675 = 0, $676 = 0, $677 = 0, $678 = 0, $679 = 0, $68 = 0;
 var $680 = 0, $681 = 0, $682 = 0, $683 = 0, $684 = 0, $685 = 0, $686 = 0, $687 = 0, $688 = 0, $689 = 0, $69 = 0, $690 = 0, $691 = 0, $692 = 0, $693 = 0, $694 = 0, $695 = 0, $696 = 0, $697 = 0, $698 = 0;
 var $699 = 0, $7 = 0, $70 = 0, $700 = 0, $701 = 0, $702 = 0, $703 = 0, $704 = 0, $705 = 0, $706 = 0, $707 = 0, $708 = 0, $709 = 0, $71 = 0, $710 = 0, $711 = 0, $712 = 0, $713 = 0, $714 = 0, $715 = 0;
 var $716 = 0, $717 = 0, $718 = 0, $719 = 0, $72 = 0, $720 = 0, $721 = 0, $722 = 0, $723 = 0, $724 = 0, $725 = 0, $726 = 0, $727 = 0, $728 = 0, $729 = 0, $73 = 0, $730 = 0, $731 = 0, $732 = 0, $733 = 0;
 var $734 = 0, $735 = 0, $736 = 0, $737 = 0, $738 = 0, $739 = 0, $74 = 0, $740 = 0, $741 = 0, $742 = 0, $743 = 0, $744 = 0, $745 = 0, $746 = 0, $747 = 0, $748 = 0, $749 = 0, $75 = 0, $750 = 0, $751 = 0;
 var $752 = 0, $753 = 0, $754 = 0, $755 = 0, $756 = 0, $757 = 0, $758 = 0, $759 = 0, $76 = 0, $760 = 0, $761 = 0, $762 = 0, $763 = 0, $764 = 0, $765 = 0, $766 = 0, $767 = 0, $768 = 0, $769 = 0, $77 = 0;
 var $770 = 0, $771 = 0, $772 = 0, $773 = 0, $774 = 0, $775 = 0, $776 = 0, $777 = 0, $778 = 0, $779 = 0, $78 = 0, $780 = 0, $781 = 0, $782 = 0, $783 = 0, $784 = 0, $785 = 0, $786 = 0, $787 = 0, $788 = 0;
 var $789 = 0, $79 = 0, $790 = 0, $791 = 0, $792 = 0, $793 = 0, $794 = 0, $795 = 0, $796 = 0, $797 = 0, $798 = 0, $799 = 0, $8 = 0, $80 = 0, $800 = 0, $801 = 0, $802 = 0, $803 = 0, $804 = 0, $805 = 0;
 var $806 = 0, $807 = 0, $808 = 0, $809 = 0, $81 = 0, $810 = 0, $811 = 0, $812 = 0, $813 = 0, $814 = 0, $815 = 0, $816 = 0, $817 = 0, $818 = 0, $819 = 0, $82 = 0, $820 = 0, $821 = 0, $822 = 0, $823 = 0;
 var $824 = 0, $825 = 0, $826 = 0, $827 = 0, $828 = 0, $829 = 0, $83 = 0, $830 = 0, $831 = 0, $832 = 0, $833 = 0, $834 = 0, $835 = 0, $836 = 0, $837 = 0, $838 = 0, $839 = 0, $84 = 0, $840 = 0, $841 = 0;
 var $842 = 0, $843 = 0, $844 = 0, $845 = 0, $846 = 0, $847 = 0, $848 = 0, $849 = 0, $85 = 0, $850 = 0, $851 = 0, $852 = 0, $853 = 0, $854 = 0, $855 = 0, $856 = 0, $857 = 0, $858 = 0, $859 = 0, $86 = 0;
 var $860 = 0, $861 = 0, $862 = 0, $863 = 0, $864 = 0, $865 = 0, $866 = 0, $867 = 0, $868 = 0, $869 = 0, $87 = 0, $870 = 0, $871 = 0, $872 = 0, $873 = 0, $874 = 0, $875 = 0, $876 = 0, $877 = 0, $878 = 0;
 var $879 = 0, $88 = 0, $880 = 0, $881 = 0, $882 = 0, $883 = 0, $884 = 0, $885 = 0, $886 = 0, $887 = 0, $888 = 0, $889 = 0, $89 = 0, $890 = 0, $891 = 0, $892 = 0, $893 = 0, $894 = 0, $895 = 0, $896 = 0;
 var $897 = 0, $898 = 0, $899 = 0, $9 = 0, $90 = 0, $900 = 0, $901 = 0, $902 = 0, $903 = 0, $904 = 0, $905 = 0, $906 = 0, $907 = 0, $908 = 0, $909 = 0, $91 = 0, $910 = 0, $911 = 0, $912 = 0, $913 = 0;
 var $914 = 0, $915 = 0, $916 = 0, $917 = 0, $918 = 0, $919 = 0, $92 = 0, $920 = 0, $921 = 0, $922 = 0, $923 = 0, $924 = 0, $925 = 0, $926 = 0, $927 = 0, $928 = 0, $929 = 0, $93 = 0, $930 = 0, $931 = 0;
 var $932 = 0, $933 = 0, $934 = 0, $935 = 0, $936 = 0, $937 = 0, $938 = 0, $939 = 0, $94 = 0, $940 = 0, $941 = 0, $942 = 0, $943 = 0, $944 = 0, $945 = 0, $946 = 0, $947 = 0, $948 = 0, $949 = 0, $95 = 0;
 var $950 = 0, $951 = 0, $952 = 0, $953 = 0, $954 = 0, $955 = 0, $956 = 0, $957 = 0, $958 = 0, $959 = 0, $96 = 0, $960 = 0, $961 = 0, $962 = 0, $963 = 0, $964 = 0, $965 = 0, $966 = 0, $967 = 0, $968 = 0;
 var $969 = 0, $97 = 0, $970 = 0, $971 = 0, $972 = 0, $973 = 0, $974 = 0, $975 = 0, $976 = 0, $977 = 0, $978 = 0, $979 = 0, $98 = 0, $980 = 0, $981 = 0, $982 = 0, $983 = 0, $984 = 0, $985 = 0, $986 = 0;
 var $987 = 0, $988 = 0, $989 = 0, $99 = 0, $990 = 0, $991 = 0, $992 = 0, $993 = 0, $994 = 0, $995 = 0, $996 = 0, $997 = 0, $998 = 0, $999 = 0, $bits = 0, $copy = 0, $from = 0, $have = 0, $hbuf = 0, $here = 0;
 var $hold = 0, $in = 0, $last = 0, $left = 0, $len = 0, $next = 0, $out = 0, $put = 0, $ret = 0, $state = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 80|0;
 $here = sp + 68|0;
 $last = sp + 64|0;
 $hbuf = sp + 72|0;
 $1 = $strm;
 $2 = $flush;
 $3 = $1;
 $4 = ($3|0)==(0|0);
 do {
  if (!($4)) {
   $5 = $1;
   $6 = (($5) + 28|0);
   $7 = HEAP32[$6>>2]|0;
   $8 = ($7|0)==(0|0);
   if (!($8)) {
    $9 = $1;
    $10 = (($9) + 12|0);
    $11 = HEAP32[$10>>2]|0;
    $12 = ($11|0)==(0|0);
    if (!($12)) {
     $13 = $1;
     $14 = HEAP32[$13>>2]|0;
     $15 = ($14|0)==(0|0);
     if ($15) {
      $16 = $1;
      $17 = (($16) + 4|0);
      $18 = HEAP32[$17>>2]|0;
      $19 = ($18|0)!=(0);
      if ($19) {
       break;
      }
     }
     $20 = $1;
     $21 = (($20) + 28|0);
     $22 = HEAP32[$21>>2]|0;
     $state = $22;
     $23 = $state;
     $24 = HEAP32[$23>>2]|0;
     $25 = ($24|0)==(11);
     if ($25) {
      $26 = $state;
      HEAP32[$26>>2] = 12;
     }
     $27 = $1;
     $28 = (($27) + 12|0);
     $29 = HEAP32[$28>>2]|0;
     $put = $29;
     $30 = $1;
     $31 = (($30) + 16|0);
     $32 = HEAP32[$31>>2]|0;
     $left = $32;
     $33 = $1;
     $34 = HEAP32[$33>>2]|0;
     $next = $34;
     $35 = $1;
     $36 = (($35) + 4|0);
     $37 = HEAP32[$36>>2]|0;
     $have = $37;
     $38 = $state;
     $39 = (($38) + 56|0);
     $40 = HEAP32[$39>>2]|0;
     $hold = $40;
     $41 = $state;
     $42 = (($41) + 60|0);
     $43 = HEAP32[$42>>2]|0;
     $bits = $43;
     $44 = $have;
     $in = $44;
     $45 = $left;
     $out = $45;
     $ret = 0;
     L13: while(1) {
      $46 = $state;
      $47 = HEAP32[$46>>2]|0;
      L15: do {
       switch ($47|0) {
       case 7:  {
        label = 167;
        break;
       }
       case 6:  {
        label = 146;
        break;
       }
       case 5:  {
        label = 129;
        break;
       }
       case 9:  {
        while(1) {
         $686 = $bits;
         $687 = ($686>>>0)<(32);
         if (!($687)) {
          break;
         }
         $688 = $have;
         $689 = ($688|0)==(0);
         if ($689) {
          label = 211;
          break L13;
         }
         $690 = $have;
         $691 = (($690) + -1)|0;
         $have = $691;
         $692 = $next;
         $693 = (($692) + 1|0);
         $next = $693;
         $694 = HEAP8[$692]|0;
         $695 = $694&255;
         $696 = $bits;
         $697 = $695 << $696;
         $698 = $hold;
         $699 = (($698) + ($697))|0;
         $hold = $699;
         $700 = $bits;
         $701 = (($700) + 8)|0;
         $bits = $701;
        }
        $702 = $hold;
        $703 = $702 >>> 24;
        $704 = $703 & 255;
        $705 = $hold;
        $706 = $705 >>> 8;
        $707 = $706 & 65280;
        $708 = (($704) + ($707))|0;
        $709 = $hold;
        $710 = $709 & 65280;
        $711 = $710 << 8;
        $712 = (($708) + ($711))|0;
        $713 = $hold;
        $714 = $713 & 255;
        $715 = $714 << 24;
        $716 = (($712) + ($715))|0;
        $717 = $state;
        $718 = (($717) + 24|0);
        HEAP32[$718>>2] = $716;
        $719 = $1;
        $720 = (($719) + 48|0);
        HEAP32[$720>>2] = $716;
        $hold = 0;
        $bits = 0;
        $721 = $state;
        HEAP32[$721>>2] = 10;
        label = 218;
        break;
       }
       case 11:  {
        label = 223;
        break;
       }
       case 12:  {
        label = 227;
        break;
       }
       case 8:  {
        label = 188;
        break;
       }
       case 10:  {
        label = 218;
        break;
       }
       case 15:  {
        label = 273;
        break;
       }
       case 13:  {
        $809 = $bits;
        $810 = $809 & 7;
        $811 = $hold;
        $812 = $811 >>> $810;
        $hold = $812;
        $813 = $bits;
        $814 = $813 & 7;
        $815 = $bits;
        $816 = (($815) - ($814))|0;
        $bits = $816;
        while(1) {
         $817 = $bits;
         $818 = ($817>>>0)<(32);
         if (!($818)) {
          break;
         }
         $819 = $have;
         $820 = ($819|0)==(0);
         if ($820) {
          label = 261;
          break L13;
         }
         $821 = $have;
         $822 = (($821) + -1)|0;
         $have = $822;
         $823 = $next;
         $824 = (($823) + 1|0);
         $next = $824;
         $825 = HEAP8[$823]|0;
         $826 = $825&255;
         $827 = $bits;
         $828 = $826 << $827;
         $829 = $hold;
         $830 = (($829) + ($828))|0;
         $hold = $830;
         $831 = $bits;
         $832 = (($831) + 8)|0;
         $bits = $832;
        }
        $833 = $hold;
        $834 = $833 & 65535;
        $835 = $hold;
        $836 = $835 >>> 16;
        $837 = $836 ^ 65535;
        $838 = ($834|0)!=($837|0);
        if ($838) {
         $839 = $1;
         $840 = (($839) + 24|0);
         HEAP32[$840>>2] = 8752;
         $841 = $state;
         HEAP32[$841>>2] = 29;
         break L15;
        }
        $842 = $hold;
        $843 = $842 & 65535;
        $844 = $state;
        $845 = (($844) + 64|0);
        HEAP32[$845>>2] = $843;
        $hold = 0;
        $bits = 0;
        $846 = $state;
        HEAP32[$846>>2] = 14;
        $847 = $2;
        $848 = ($847|0)==(6);
        if ($848) {
         label = 270;
         break L13;
        }
        label = 272;
        break;
       }
       case 16:  {
        while(1) {
         $886 = $bits;
         $887 = ($886>>>0)<(14);
         if (!($887)) {
          break;
         }
         $888 = $have;
         $889 = ($888|0)==(0);
         if ($889) {
          label = 287;
          break L13;
         }
         $890 = $have;
         $891 = (($890) + -1)|0;
         $have = $891;
         $892 = $next;
         $893 = (($892) + 1|0);
         $next = $893;
         $894 = HEAP8[$892]|0;
         $895 = $894&255;
         $896 = $bits;
         $897 = $895 << $896;
         $898 = $hold;
         $899 = (($898) + ($897))|0;
         $hold = $899;
         $900 = $bits;
         $901 = (($900) + 8)|0;
         $bits = $901;
        }
        $902 = $hold;
        $903 = $902 & 31;
        $904 = (($903) + 257)|0;
        $905 = $state;
        $906 = (($905) + 96|0);
        HEAP32[$906>>2] = $904;
        $907 = $hold;
        $908 = $907 >>> 5;
        $hold = $908;
        $909 = $bits;
        $910 = (($909) - 5)|0;
        $bits = $910;
        $911 = $hold;
        $912 = $911 & 31;
        $913 = (($912) + 1)|0;
        $914 = $state;
        $915 = (($914) + 100|0);
        HEAP32[$915>>2] = $913;
        $916 = $hold;
        $917 = $916 >>> 5;
        $hold = $917;
        $918 = $bits;
        $919 = (($918) - 5)|0;
        $bits = $919;
        $920 = $hold;
        $921 = $920 & 15;
        $922 = (($921) + 4)|0;
        $923 = $state;
        $924 = (($923) + 92|0);
        HEAP32[$924>>2] = $922;
        $925 = $hold;
        $926 = $925 >>> 4;
        $hold = $926;
        $927 = $bits;
        $928 = (($927) - 4)|0;
        $bits = $928;
        $929 = $state;
        $930 = (($929) + 96|0);
        $931 = HEAP32[$930>>2]|0;
        $932 = ($931>>>0)>(286);
        if (!($932)) {
         $933 = $state;
         $934 = (($933) + 100|0);
         $935 = HEAP32[$934>>2]|0;
         $936 = ($935>>>0)>(30);
         if (!($936)) {
          $940 = $state;
          $941 = (($940) + 104|0);
          HEAP32[$941>>2] = 0;
          $942 = $state;
          HEAP32[$942>>2] = 17;
          label = 301;
          break L15;
         }
        }
        $937 = $1;
        $938 = (($937) + 24|0);
        HEAP32[$938>>2] = 8784;
        $939 = $state;
        HEAP32[$939>>2] = 29;
        break;
       }
       case 14:  {
        label = 272;
        break;
       }
       case 17:  {
        label = 301;
        break;
       }
       case 18:  {
        label = 321;
        break;
       }
       case 1:  {
        while(1) {
         $160 = $bits;
         $161 = ($160>>>0)<(16);
         if (!($161)) {
          break;
         }
         $162 = $have;
         $163 = ($162|0)==(0);
         if ($163) {
          label = 53;
          break L13;
         }
         $164 = $have;
         $165 = (($164) + -1)|0;
         $have = $165;
         $166 = $next;
         $167 = (($166) + 1|0);
         $next = $167;
         $168 = HEAP8[$166]|0;
         $169 = $168&255;
         $170 = $bits;
         $171 = $169 << $170;
         $172 = $hold;
         $173 = (($172) + ($171))|0;
         $hold = $173;
         $174 = $bits;
         $175 = (($174) + 8)|0;
         $bits = $175;
        }
        $176 = $hold;
        $177 = $state;
        $178 = (($177) + 16|0);
        HEAP32[$178>>2] = $176;
        $179 = $state;
        $180 = (($179) + 16|0);
        $181 = HEAP32[$180>>2]|0;
        $182 = $181 & 255;
        $183 = ($182|0)!=(8);
        if ($183) {
         $184 = $1;
         $185 = (($184) + 24|0);
         HEAP32[$185>>2] = 8616;
         $186 = $state;
         HEAP32[$186>>2] = 29;
         break L15;
        }
        $187 = $state;
        $188 = (($187) + 16|0);
        $189 = HEAP32[$188>>2]|0;
        $190 = $189 & 57344;
        $191 = ($190|0)!=(0);
        if ($191) {
         $192 = $1;
         $193 = (($192) + 24|0);
         HEAP32[$193>>2] = 8672;
         $194 = $state;
         HEAP32[$194>>2] = 29;
         break L15;
        }
        $195 = $state;
        $196 = (($195) + 32|0);
        $197 = HEAP32[$196>>2]|0;
        $198 = ($197|0)!=(0|0);
        if ($198) {
         $199 = $hold;
         $200 = $199 >>> 8;
         $201 = $200 & 1;
         $202 = $state;
         $203 = (($202) + 32|0);
         $204 = HEAP32[$203>>2]|0;
         HEAP32[$204>>2] = $201;
        }
        $205 = $state;
        $206 = (($205) + 16|0);
        $207 = HEAP32[$206>>2]|0;
        $208 = $207 & 512;
        $209 = ($208|0)!=(0);
        if ($209) {
         $210 = $hold;
         $211 = $210&255;
         HEAP8[$hbuf] = $211;
         $212 = $hold;
         $213 = $212 >>> 8;
         $214 = $213&255;
         $215 = (($hbuf) + 1|0);
         HEAP8[$215] = $214;
         $216 = $state;
         $217 = (($216) + 24|0);
         $218 = HEAP32[$217>>2]|0;
         $219 = (_crc32($218,$hbuf,2)|0);
         $220 = $state;
         $221 = (($220) + 24|0);
         HEAP32[$221>>2] = $219;
        }
        $hold = 0;
        $bits = 0;
        $222 = $state;
        HEAP32[$222>>2] = 2;
        label = 70;
        break;
       }
       case 0:  {
        $48 = $state;
        $49 = (($48) + 8|0);
        $50 = HEAP32[$49>>2]|0;
        $51 = ($50|0)==(0);
        if ($51) {
         $52 = $state;
         HEAP32[$52>>2] = 12;
         break L15;
        }
        while(1) {
         $53 = $bits;
         $54 = ($53>>>0)<(16);
         if (!($54)) {
          break;
         }
         $55 = $have;
         $56 = ($55|0)==(0);
         if ($56) {
          label = 20;
          break L13;
         }
         $57 = $have;
         $58 = (($57) + -1)|0;
         $have = $58;
         $59 = $next;
         $60 = (($59) + 1|0);
         $next = $60;
         $61 = HEAP8[$59]|0;
         $62 = $61&255;
         $63 = $bits;
         $64 = $62 << $63;
         $65 = $hold;
         $66 = (($65) + ($64))|0;
         $hold = $66;
         $67 = $bits;
         $68 = (($67) + 8)|0;
         $bits = $68;
        }
        $69 = $state;
        $70 = (($69) + 8|0);
        $71 = HEAP32[$70>>2]|0;
        $72 = $71 & 2;
        $73 = ($72|0)!=(0);
        if ($73) {
         $74 = $hold;
         $75 = ($74|0)==(35615);
         if ($75) {
          $76 = (_crc32(0,0,0)|0);
          $77 = $state;
          $78 = (($77) + 24|0);
          HEAP32[$78>>2] = $76;
          $79 = $hold;
          $80 = $79&255;
          HEAP8[$hbuf] = $80;
          $81 = $hold;
          $82 = $81 >>> 8;
          $83 = $82&255;
          $84 = (($hbuf) + 1|0);
          HEAP8[$84] = $83;
          $85 = $state;
          $86 = (($85) + 24|0);
          $87 = HEAP32[$86>>2]|0;
          $88 = (_crc32($87,$hbuf,2)|0);
          $89 = $state;
          $90 = (($89) + 24|0);
          HEAP32[$90>>2] = $88;
          $hold = 0;
          $bits = 0;
          $91 = $state;
          HEAP32[$91>>2] = 1;
          break L15;
         }
        }
        $92 = $state;
        $93 = (($92) + 16|0);
        HEAP32[$93>>2] = 0;
        $94 = $state;
        $95 = (($94) + 32|0);
        $96 = HEAP32[$95>>2]|0;
        $97 = ($96|0)!=(0|0);
        if ($97) {
         $98 = $state;
         $99 = (($98) + 32|0);
         $100 = HEAP32[$99>>2]|0;
         $101 = (($100) + 48|0);
         HEAP32[$101>>2] = -1;
        }
        $102 = $state;
        $103 = (($102) + 8|0);
        $104 = HEAP32[$103>>2]|0;
        $105 = $104 & 1;
        $106 = ($105|0)!=(0);
        if ($106) {
         $107 = $hold;
         $108 = $107 & 255;
         $109 = $108 << 8;
         $110 = $hold;
         $111 = $110 >>> 8;
         $112 = (($109) + ($111))|0;
         $113 = (($112>>>0) % 31)&-1;
         $114 = ($113|0)!=(0);
         if (!($114)) {
          $118 = $hold;
          $119 = $118 & 15;
          $120 = ($119|0)!=(8);
          if ($120) {
           $121 = $1;
           $122 = (($121) + 24|0);
           HEAP32[$122>>2] = 8616;
           $123 = $state;
           HEAP32[$123>>2] = 29;
           break L15;
          }
          $124 = $hold;
          $125 = $124 >>> 4;
          $hold = $125;
          $126 = $bits;
          $127 = (($126) - 4)|0;
          $bits = $127;
          $128 = $hold;
          $129 = $128 & 15;
          $130 = (($129) + 8)|0;
          $len = $130;
          $131 = $state;
          $132 = (($131) + 36|0);
          $133 = HEAP32[$132>>2]|0;
          $134 = ($133|0)==(0);
          do {
           if ($134) {
            $135 = $len;
            $136 = $state;
            $137 = (($136) + 36|0);
            HEAP32[$137>>2] = $135;
           } else {
            $138 = $len;
            $139 = $state;
            $140 = (($139) + 36|0);
            $141 = HEAP32[$140>>2]|0;
            $142 = ($138>>>0)>($141>>>0);
            if ($142) {
             $143 = $1;
             $144 = (($143) + 24|0);
             HEAP32[$144>>2] = 8648;
             $145 = $state;
             HEAP32[$145>>2] = 29;
             break L15;
            } else {
             break;
            }
           }
          } while(0);
          $146 = $len;
          $147 = 1 << $146;
          $148 = $state;
          $149 = (($148) + 20|0);
          HEAP32[$149>>2] = $147;
          $150 = (_adler32(0,0,0)|0);
          $151 = $state;
          $152 = (($151) + 24|0);
          HEAP32[$152>>2] = $150;
          $153 = $1;
          $154 = (($153) + 48|0);
          HEAP32[$154>>2] = $150;
          $155 = $hold;
          $156 = $155 & 512;
          $157 = ($156|0)!=(0);
          $158 = $157 ? 9 : 11;
          $159 = $state;
          HEAP32[$159>>2] = $158;
          $hold = 0;
          $bits = 0;
          break L15;
         }
        }
        $115 = $1;
        $116 = (($115) + 24|0);
        HEAP32[$116>>2] = 8592;
        $117 = $state;
        HEAP32[$117>>2] = 29;
        break;
       }
       case 2:  {
        label = 70;
        break;
       }
       case 3:  {
        label = 88;
        break;
       }
       case 4:  {
        label = 106;
        break;
       }
       case 28:  {
        label = 559;
        break L13;
        break;
       }
       case 31:  {
        label = 562;
        break L13;
        break;
       }
       case 29:  {
        label = 560;
        break L13;
        break;
       }
       case 27:  {
        label = 542;
        break;
       }
       case 30:  {
        label = 561;
        break L13;
        break;
       }
       case 19:  {
        label = 400;
        break;
       }
       case 20:  {
        label = 401;
        break;
       }
       case 22:  {
        label = 454;
        break;
       }
       case 23:  {
        label = 479;
        break;
       }
       case 21:  {
        label = 440;
        break;
       }
       case 26:  {
        $1864 = $state;
        $1865 = (($1864) + 8|0);
        $1866 = HEAP32[$1865>>2]|0;
        $1867 = ($1866|0)!=(0);
        if ($1867) {
         while(1) {
          $1868 = $bits;
          $1869 = ($1868>>>0)<(32);
          if (!($1869)) {
           break;
          }
          $1870 = $have;
          $1871 = ($1870|0)==(0);
          if ($1871) {
           label = 524;
           break L13;
          }
          $1872 = $have;
          $1873 = (($1872) + -1)|0;
          $have = $1873;
          $1874 = $next;
          $1875 = (($1874) + 1|0);
          $next = $1875;
          $1876 = HEAP8[$1874]|0;
          $1877 = $1876&255;
          $1878 = $bits;
          $1879 = $1877 << $1878;
          $1880 = $hold;
          $1881 = (($1880) + ($1879))|0;
          $hold = $1881;
          $1882 = $bits;
          $1883 = (($1882) + 8)|0;
          $bits = $1883;
         }
         $1884 = $left;
         $1885 = $out;
         $1886 = (($1885) - ($1884))|0;
         $out = $1886;
         $1887 = $out;
         $1888 = $1;
         $1889 = (($1888) + 20|0);
         $1890 = HEAP32[$1889>>2]|0;
         $1891 = (($1890) + ($1887))|0;
         HEAP32[$1889>>2] = $1891;
         $1892 = $out;
         $1893 = $state;
         $1894 = (($1893) + 28|0);
         $1895 = HEAP32[$1894>>2]|0;
         $1896 = (($1895) + ($1892))|0;
         HEAP32[$1894>>2] = $1896;
         $1897 = $out;
         $1898 = ($1897|0)!=(0);
         if ($1898) {
          $1899 = $state;
          $1900 = (($1899) + 16|0);
          $1901 = HEAP32[$1900>>2]|0;
          $1902 = ($1901|0)!=(0);
          if ($1902) {
           $1903 = $state;
           $1904 = (($1903) + 24|0);
           $1905 = HEAP32[$1904>>2]|0;
           $1906 = $put;
           $1907 = $out;
           $1908 = (0 - ($1907))|0;
           $1909 = (($1906) + ($1908)|0);
           $1910 = $out;
           $1911 = (_crc32($1905,$1909,$1910)|0);
           $1923 = $1911;
          } else {
           $1912 = $state;
           $1913 = (($1912) + 24|0);
           $1914 = HEAP32[$1913>>2]|0;
           $1915 = $put;
           $1916 = $out;
           $1917 = (0 - ($1916))|0;
           $1918 = (($1915) + ($1917)|0);
           $1919 = $out;
           $1920 = (_adler32($1914,$1918,$1919)|0);
           $1923 = $1920;
          }
          $1921 = $state;
          $1922 = (($1921) + 24|0);
          HEAP32[$1922>>2] = $1923;
          $1924 = $1;
          $1925 = (($1924) + 48|0);
          HEAP32[$1925>>2] = $1923;
         }
         $1926 = $left;
         $out = $1926;
         $1927 = $state;
         $1928 = (($1927) + 16|0);
         $1929 = HEAP32[$1928>>2]|0;
         $1930 = ($1929|0)!=(0);
         if ($1930) {
          $1931 = $hold;
          $1951 = $1931;
         } else {
          $1932 = $hold;
          $1933 = $1932 >>> 24;
          $1934 = $1933 & 255;
          $1935 = $hold;
          $1936 = $1935 >>> 8;
          $1937 = $1936 & 65280;
          $1938 = (($1934) + ($1937))|0;
          $1939 = $hold;
          $1940 = $1939 & 65280;
          $1941 = $1940 << 8;
          $1942 = (($1938) + ($1941))|0;
          $1943 = $hold;
          $1944 = $1943 & 255;
          $1945 = $1944 << 24;
          $1946 = (($1942) + ($1945))|0;
          $1951 = $1946;
         }
         $1947 = $state;
         $1948 = (($1947) + 24|0);
         $1949 = HEAP32[$1948>>2]|0;
         $1950 = ($1951|0)!=($1949|0);
         if ($1950) {
          $1952 = $1;
          $1953 = (($1952) + 24|0);
          HEAP32[$1953>>2] = 8984;
          $1954 = $state;
          HEAP32[$1954>>2] = 29;
          break L15;
         }
         $hold = 0;
         $bits = 0;
        }
        $1955 = $state;
        HEAP32[$1955>>2] = 27;
        label = 542;
        break;
       }
       case 25:  {
        $1853 = $left;
        $1854 = ($1853|0)==(0);
        if ($1854) {
         label = 516;
         break L13;
        }
        $1855 = $state;
        $1856 = (($1855) + 64|0);
        $1857 = HEAP32[$1856>>2]|0;
        $1858 = $1857&255;
        $1859 = $put;
        $1860 = (($1859) + 1|0);
        $put = $1860;
        HEAP8[$1859] = $1858;
        $1861 = $left;
        $1862 = (($1861) + -1)|0;
        $left = $1862;
        $1863 = $state;
        HEAP32[$1863>>2] = 20;
        break;
       }
       case 24:  {
        label = 493;
        break;
       }
       default: {
        label = 563;
        break L13;
       }
       }
      } while(0);
      do {
       if ((label|0) == 70) {
        label = 0;
        while(1) {
         $223 = $bits;
         $224 = ($223>>>0)<(32);
         if (!($224)) {
          break;
         }
         $225 = $have;
         $226 = ($225|0)==(0);
         if ($226) {
          label = 75;
          break L13;
         }
         $227 = $have;
         $228 = (($227) + -1)|0;
         $have = $228;
         $229 = $next;
         $230 = (($229) + 1|0);
         $next = $230;
         $231 = HEAP8[$229]|0;
         $232 = $231&255;
         $233 = $bits;
         $234 = $232 << $233;
         $235 = $hold;
         $236 = (($235) + ($234))|0;
         $hold = $236;
         $237 = $bits;
         $238 = (($237) + 8)|0;
         $bits = $238;
        }
        $239 = $state;
        $240 = (($239) + 32|0);
        $241 = HEAP32[$240>>2]|0;
        $242 = ($241|0)!=(0|0);
        if ($242) {
         $243 = $hold;
         $244 = $state;
         $245 = (($244) + 32|0);
         $246 = HEAP32[$245>>2]|0;
         $247 = (($246) + 4|0);
         HEAP32[$247>>2] = $243;
        }
        $248 = $state;
        $249 = (($248) + 16|0);
        $250 = HEAP32[$249>>2]|0;
        $251 = $250 & 512;
        $252 = ($251|0)!=(0);
        if ($252) {
         $253 = $hold;
         $254 = $253&255;
         HEAP8[$hbuf] = $254;
         $255 = $hold;
         $256 = $255 >>> 8;
         $257 = $256&255;
         $258 = (($hbuf) + 1|0);
         HEAP8[$258] = $257;
         $259 = $hold;
         $260 = $259 >>> 16;
         $261 = $260&255;
         $262 = (($hbuf) + 2|0);
         HEAP8[$262] = $261;
         $263 = $hold;
         $264 = $263 >>> 24;
         $265 = $264&255;
         $266 = (($hbuf) + 3|0);
         HEAP8[$266] = $265;
         $267 = $state;
         $268 = (($267) + 24|0);
         $269 = HEAP32[$268>>2]|0;
         $270 = (_crc32($269,$hbuf,4)|0);
         $271 = $state;
         $272 = (($271) + 24|0);
         HEAP32[$272>>2] = $270;
        }
        $hold = 0;
        $bits = 0;
        $273 = $state;
        HEAP32[$273>>2] = 3;
        label = 88;
       }
       else if ((label|0) == 218) {
        label = 0;
        $722 = $state;
        $723 = (($722) + 12|0);
        $724 = HEAP32[$723>>2]|0;
        $725 = ($724|0)==(0);
        if ($725) {
         label = 219;
         break L13;
        }
        $743 = (_adler32(0,0,0)|0);
        $744 = $state;
        $745 = (($744) + 24|0);
        HEAP32[$745>>2] = $743;
        $746 = $1;
        $747 = (($746) + 48|0);
        HEAP32[$747>>2] = $743;
        $748 = $state;
        HEAP32[$748>>2] = 11;
        label = 223;
       }
       else if ((label|0) == 272) {
        label = 0;
        $849 = $state;
        HEAP32[$849>>2] = 15;
        label = 273;
       }
       else if ((label|0) == 301) {
        label = 0;
        while(1) {
         $943 = $state;
         $944 = (($943) + 104|0);
         $945 = HEAP32[$944>>2]|0;
         $946 = $state;
         $947 = (($946) + 92|0);
         $948 = HEAP32[$947>>2]|0;
         $949 = ($945>>>0)<($948>>>0);
         if (!($949)) {
          break;
         }
         while(1) {
          $950 = $bits;
          $951 = ($950>>>0)<(3);
          if (!($951)) {
           break;
          }
          $952 = $have;
          $953 = ($952|0)==(0);
          if ($953) {
           label = 308;
           break L13;
          }
          $954 = $have;
          $955 = (($954) + -1)|0;
          $have = $955;
          $956 = $next;
          $957 = (($956) + 1|0);
          $next = $957;
          $958 = HEAP8[$956]|0;
          $959 = $958&255;
          $960 = $bits;
          $961 = $959 << $960;
          $962 = $hold;
          $963 = (($962) + ($961))|0;
          $hold = $963;
          $964 = $bits;
          $965 = (($964) + 8)|0;
          $bits = $965;
         }
         $966 = $hold;
         $967 = $966 & 7;
         $968 = $967&65535;
         $969 = $state;
         $970 = (($969) + 104|0);
         $971 = HEAP32[$970>>2]|0;
         $972 = (($971) + 1)|0;
         HEAP32[$970>>2] = $972;
         $973 = (8552 + ($971<<1)|0);
         $974 = HEAP16[$973>>1]|0;
         $975 = $974&65535;
         $976 = $state;
         $977 = (($976) + 112|0);
         $978 = (($977) + ($975<<1)|0);
         HEAP16[$978>>1] = $968;
         $979 = $hold;
         $980 = $979 >>> 3;
         $hold = $980;
         $981 = $bits;
         $982 = (($981) - 3)|0;
         $bits = $982;
        }
        while(1) {
         $983 = $state;
         $984 = (($983) + 104|0);
         $985 = HEAP32[$984>>2]|0;
         $986 = ($985>>>0)<(19);
         if (!($986)) {
          break;
         }
         $987 = $state;
         $988 = (($987) + 104|0);
         $989 = HEAP32[$988>>2]|0;
         $990 = (($989) + 1)|0;
         HEAP32[$988>>2] = $990;
         $991 = (8552 + ($989<<1)|0);
         $992 = HEAP16[$991>>1]|0;
         $993 = $992&65535;
         $994 = $state;
         $995 = (($994) + 112|0);
         $996 = (($995) + ($993<<1)|0);
         HEAP16[$996>>1] = 0;
        }
        $997 = $state;
        $998 = (($997) + 1328|0);
        $999 = $state;
        $1000 = (($999) + 108|0);
        HEAP32[$1000>>2] = $998;
        $1001 = $state;
        $1002 = (($1001) + 108|0);
        $1003 = HEAP32[$1002>>2]|0;
        $1004 = $state;
        $1005 = (($1004) + 76|0);
        HEAP32[$1005>>2] = $1003;
        $1006 = $state;
        $1007 = (($1006) + 84|0);
        HEAP32[$1007>>2] = 7;
        $1008 = $state;
        $1009 = (($1008) + 112|0);
        $1010 = $state;
        $1011 = (($1010) + 108|0);
        $1012 = $state;
        $1013 = (($1012) + 84|0);
        $1014 = $state;
        $1015 = (($1014) + 752|0);
        $1016 = (_inflate_table(0,$1009,19,$1011,$1013,$1015)|0);
        $ret = $1016;
        $1017 = $ret;
        $1018 = ($1017|0)!=(0);
        if ($1018) {
         $1019 = $1;
         $1020 = (($1019) + 24|0);
         HEAP32[$1020>>2] = 8824;
         $1021 = $state;
         HEAP32[$1021>>2] = 29;
         break;
        } else {
         $1022 = $state;
         $1023 = (($1022) + 104|0);
         HEAP32[$1023>>2] = 0;
         $1024 = $state;
         HEAP32[$1024>>2] = 18;
         label = 321;
         break;
        }
       }
       else if ((label|0) == 542) {
        label = 0;
        $1956 = $state;
        $1957 = (($1956) + 8|0);
        $1958 = HEAP32[$1957>>2]|0;
        $1959 = ($1958|0)!=(0);
        if (!($1959)) {
         label = 558;
         break L13;
        }
        $1960 = $state;
        $1961 = (($1960) + 16|0);
        $1962 = HEAP32[$1961>>2]|0;
        $1963 = ($1962|0)!=(0);
        if (!($1963)) {
         label = 558;
         break L13;
        }
        while(1) {
         $1964 = $bits;
         $1965 = ($1964>>>0)<(32);
         if (!($1965)) {
          break;
         }
         $1966 = $have;
         $1967 = ($1966|0)==(0);
         if ($1967) {
          label = 549;
          break L13;
         }
         $1968 = $have;
         $1969 = (($1968) + -1)|0;
         $have = $1969;
         $1970 = $next;
         $1971 = (($1970) + 1|0);
         $next = $1971;
         $1972 = HEAP8[$1970]|0;
         $1973 = $1972&255;
         $1974 = $bits;
         $1975 = $1973 << $1974;
         $1976 = $hold;
         $1977 = (($1976) + ($1975))|0;
         $hold = $1977;
         $1978 = $bits;
         $1979 = (($1978) + 8)|0;
         $bits = $1979;
        }
        $1980 = $hold;
        $1981 = $state;
        $1982 = (($1981) + 28|0);
        $1983 = HEAP32[$1982>>2]|0;
        $1984 = ($1980|0)!=($1983|0);
        if (!($1984)) {
         label = 555;
         break L13;
        }
        $1985 = $1;
        $1986 = (($1985) + 24|0);
        HEAP32[$1986>>2] = 9008;
        $1987 = $state;
        HEAP32[$1987>>2] = 29;
       }
      } while(0);
      do {
       if ((label|0) == 88) {
        label = 0;
        while(1) {
         $274 = $bits;
         $275 = ($274>>>0)<(16);
         if (!($275)) {
          break;
         }
         $276 = $have;
         $277 = ($276|0)==(0);
         if ($277) {
          label = 93;
          break L13;
         }
         $278 = $have;
         $279 = (($278) + -1)|0;
         $have = $279;
         $280 = $next;
         $281 = (($280) + 1|0);
         $next = $281;
         $282 = HEAP8[$280]|0;
         $283 = $282&255;
         $284 = $bits;
         $285 = $283 << $284;
         $286 = $hold;
         $287 = (($286) + ($285))|0;
         $hold = $287;
         $288 = $bits;
         $289 = (($288) + 8)|0;
         $bits = $289;
        }
        $290 = $state;
        $291 = (($290) + 32|0);
        $292 = HEAP32[$291>>2]|0;
        $293 = ($292|0)!=(0|0);
        if ($293) {
         $294 = $hold;
         $295 = $294 & 255;
         $296 = $state;
         $297 = (($296) + 32|0);
         $298 = HEAP32[$297>>2]|0;
         $299 = (($298) + 8|0);
         HEAP32[$299>>2] = $295;
         $300 = $hold;
         $301 = $300 >>> 8;
         $302 = $state;
         $303 = (($302) + 32|0);
         $304 = HEAP32[$303>>2]|0;
         $305 = (($304) + 12|0);
         HEAP32[$305>>2] = $301;
        }
        $306 = $state;
        $307 = (($306) + 16|0);
        $308 = HEAP32[$307>>2]|0;
        $309 = $308 & 512;
        $310 = ($309|0)!=(0);
        if ($310) {
         $311 = $hold;
         $312 = $311&255;
         HEAP8[$hbuf] = $312;
         $313 = $hold;
         $314 = $313 >>> 8;
         $315 = $314&255;
         $316 = (($hbuf) + 1|0);
         HEAP8[$316] = $315;
         $317 = $state;
         $318 = (($317) + 24|0);
         $319 = HEAP32[$318>>2]|0;
         $320 = (_crc32($319,$hbuf,2)|0);
         $321 = $state;
         $322 = (($321) + 24|0);
         HEAP32[$322>>2] = $320;
        }
        $hold = 0;
        $bits = 0;
        $323 = $state;
        HEAP32[$323>>2] = 4;
        label = 106;
       }
       else if ((label|0) == 223) {
        label = 0;
        $749 = $2;
        $750 = ($749|0)==(5);
        if ($750) {
         label = 225;
         break L13;
        }
        $751 = $2;
        $752 = ($751|0)==(6);
        if ($752) {
         label = 225;
         break L13;
        }
        label = 227;
       }
       else if ((label|0) == 273) {
        label = 0;
        $850 = $state;
        $851 = (($850) + 64|0);
        $852 = HEAP32[$851>>2]|0;
        $copy = $852;
        $853 = $copy;
        $854 = ($853|0)!=(0);
        if (!($854)) {
         $885 = $state;
         HEAP32[$885>>2] = 11;
         break;
        }
        $855 = $copy;
        $856 = $have;
        $857 = ($855>>>0)>($856>>>0);
        if ($857) {
         $858 = $have;
         $copy = $858;
        }
        $859 = $copy;
        $860 = $left;
        $861 = ($859>>>0)>($860>>>0);
        if ($861) {
         $862 = $left;
         $copy = $862;
        }
        $863 = $copy;
        $864 = ($863|0)==(0);
        if ($864) {
         label = 279;
         break L13;
        }
        $865 = $put;
        $866 = $next;
        $867 = $copy;
        _memcpy(($865|0),($866|0),($867|0))|0;
        $868 = $copy;
        $869 = $have;
        $870 = (($869) - ($868))|0;
        $have = $870;
        $871 = $copy;
        $872 = $next;
        $873 = (($872) + ($871)|0);
        $next = $873;
        $874 = $copy;
        $875 = $left;
        $876 = (($875) - ($874))|0;
        $left = $876;
        $877 = $copy;
        $878 = $put;
        $879 = (($878) + ($877)|0);
        $put = $879;
        $880 = $copy;
        $881 = $state;
        $882 = (($881) + 64|0);
        $883 = HEAP32[$882>>2]|0;
        $884 = (($883) - ($880))|0;
        HEAP32[$882>>2] = $884;
       }
       else if ((label|0) == 321) {
        label = 0;
        while(1) {
         $1025 = $state;
         $1026 = (($1025) + 104|0);
         $1027 = HEAP32[$1026>>2]|0;
         $1028 = $state;
         $1029 = (($1028) + 96|0);
         $1030 = HEAP32[$1029>>2]|0;
         $1031 = $state;
         $1032 = (($1031) + 100|0);
         $1033 = HEAP32[$1032>>2]|0;
         $1034 = (($1030) + ($1033))|0;
         $1035 = ($1027>>>0)<($1034>>>0);
         if (!($1035)) {
          break;
         }
         while(1) {
          $1036 = $hold;
          $1037 = $state;
          $1038 = (($1037) + 84|0);
          $1039 = HEAP32[$1038>>2]|0;
          $1040 = 1 << $1039;
          $1041 = (($1040) - 1)|0;
          $1042 = $1036 & $1041;
          $1043 = $state;
          $1044 = (($1043) + 76|0);
          $1045 = HEAP32[$1044>>2]|0;
          $1046 = (($1045) + ($1042<<2)|0);
          ;HEAP16[$here+0>>1]=HEAP16[$1046+0>>1]|0;HEAP16[$here+2>>1]=HEAP16[$1046+2>>1]|0;
          $1047 = (($here) + 1|0);
          $1048 = HEAP8[$1047]|0;
          $1049 = $1048&255;
          $1050 = $bits;
          $1051 = ($1049>>>0)<=($1050>>>0);
          if ($1051) {
           break;
          }
          $1052 = $have;
          $1053 = ($1052|0)==(0);
          if ($1053) {
           label = 328;
           break L13;
          }
          $1054 = $have;
          $1055 = (($1054) + -1)|0;
          $have = $1055;
          $1056 = $next;
          $1057 = (($1056) + 1|0);
          $next = $1057;
          $1058 = HEAP8[$1056]|0;
          $1059 = $1058&255;
          $1060 = $bits;
          $1061 = $1059 << $1060;
          $1062 = $hold;
          $1063 = (($1062) + ($1061))|0;
          $hold = $1063;
          $1064 = $bits;
          $1065 = (($1064) + 8)|0;
          $bits = $1065;
         }
         $1066 = (($here) + 2|0);
         $1067 = HEAP16[$1066>>1]|0;
         $1068 = $1067&65535;
         $1069 = ($1068|0)<(16);
         if ($1069) {
          $1070 = (($here) + 1|0);
          $1071 = HEAP8[$1070]|0;
          $1072 = $1071&255;
          $1073 = $hold;
          $1074 = $1073 >>> $1072;
          $hold = $1074;
          $1075 = (($here) + 1|0);
          $1076 = HEAP8[$1075]|0;
          $1077 = $1076&255;
          $1078 = $bits;
          $1079 = (($1078) - ($1077))|0;
          $bits = $1079;
          $1080 = (($here) + 2|0);
          $1081 = HEAP16[$1080>>1]|0;
          $1082 = $state;
          $1083 = (($1082) + 104|0);
          $1084 = HEAP32[$1083>>2]|0;
          $1085 = (($1084) + 1)|0;
          HEAP32[$1083>>2] = $1085;
          $1086 = $state;
          $1087 = (($1086) + 112|0);
          $1088 = (($1087) + ($1084<<1)|0);
          HEAP16[$1088>>1] = $1081;
         } else {
          $1089 = (($here) + 2|0);
          $1090 = HEAP16[$1089>>1]|0;
          $1091 = $1090&65535;
          $1092 = ($1091|0)==(16);
          if ($1092) {
           while(1) {
            $1093 = $bits;
            $1094 = (($here) + 1|0);
            $1095 = HEAP8[$1094]|0;
            $1096 = $1095&255;
            $1097 = (($1096) + 2)|0;
            $1098 = ($1093>>>0)<($1097>>>0);
            if (!($1098)) {
             break;
            }
            $1099 = $have;
            $1100 = ($1099|0)==(0);
            if ($1100) {
             label = 341;
             break L13;
            }
            $1101 = $have;
            $1102 = (($1101) + -1)|0;
            $have = $1102;
            $1103 = $next;
            $1104 = (($1103) + 1|0);
            $next = $1104;
            $1105 = HEAP8[$1103]|0;
            $1106 = $1105&255;
            $1107 = $bits;
            $1108 = $1106 << $1107;
            $1109 = $hold;
            $1110 = (($1109) + ($1108))|0;
            $hold = $1110;
            $1111 = $bits;
            $1112 = (($1111) + 8)|0;
            $bits = $1112;
           }
           $1113 = (($here) + 1|0);
           $1114 = HEAP8[$1113]|0;
           $1115 = $1114&255;
           $1116 = $hold;
           $1117 = $1116 >>> $1115;
           $hold = $1117;
           $1118 = (($here) + 1|0);
           $1119 = HEAP8[$1118]|0;
           $1120 = $1119&255;
           $1121 = $bits;
           $1122 = (($1121) - ($1120))|0;
           $bits = $1122;
           $1123 = $state;
           $1124 = (($1123) + 104|0);
           $1125 = HEAP32[$1124>>2]|0;
           $1126 = ($1125|0)==(0);
           if ($1126) {
            label = 348;
            break;
           }
           $1130 = $state;
           $1131 = (($1130) + 104|0);
           $1132 = HEAP32[$1131>>2]|0;
           $1133 = (($1132) - 1)|0;
           $1134 = $state;
           $1135 = (($1134) + 112|0);
           $1136 = (($1135) + ($1133<<1)|0);
           $1137 = HEAP16[$1136>>1]|0;
           $1138 = $1137&65535;
           $len = $1138;
           $1139 = $hold;
           $1140 = $1139 & 3;
           $1141 = (3 + ($1140))|0;
           $copy = $1141;
           $1142 = $hold;
           $1143 = $1142 >>> 2;
           $hold = $1143;
           $1144 = $bits;
           $1145 = (($1144) - 2)|0;
           $bits = $1145;
          } else {
           $1146 = (($here) + 2|0);
           $1147 = HEAP16[$1146>>1]|0;
           $1148 = $1147&65535;
           $1149 = ($1148|0)==(17);
           if ($1149) {
            while(1) {
             $1150 = $bits;
             $1151 = (($here) + 1|0);
             $1152 = HEAP8[$1151]|0;
             $1153 = $1152&255;
             $1154 = (($1153) + 3)|0;
             $1155 = ($1150>>>0)<($1154>>>0);
             if (!($1155)) {
              break;
             }
             $1156 = $have;
             $1157 = ($1156|0)==(0);
             if ($1157) {
              label = 358;
              break L13;
             }
             $1158 = $have;
             $1159 = (($1158) + -1)|0;
             $have = $1159;
             $1160 = $next;
             $1161 = (($1160) + 1|0);
             $next = $1161;
             $1162 = HEAP8[$1160]|0;
             $1163 = $1162&255;
             $1164 = $bits;
             $1165 = $1163 << $1164;
             $1166 = $hold;
             $1167 = (($1166) + ($1165))|0;
             $hold = $1167;
             $1168 = $bits;
             $1169 = (($1168) + 8)|0;
             $bits = $1169;
            }
            $1170 = (($here) + 1|0);
            $1171 = HEAP8[$1170]|0;
            $1172 = $1171&255;
            $1173 = $hold;
            $1174 = $1173 >>> $1172;
            $hold = $1174;
            $1175 = (($here) + 1|0);
            $1176 = HEAP8[$1175]|0;
            $1177 = $1176&255;
            $1178 = $bits;
            $1179 = (($1178) - ($1177))|0;
            $bits = $1179;
            $len = 0;
            $1180 = $hold;
            $1181 = $1180 & 7;
            $1182 = (3 + ($1181))|0;
            $copy = $1182;
            $1183 = $hold;
            $1184 = $1183 >>> 3;
            $hold = $1184;
            $1185 = $bits;
            $1186 = (($1185) - 3)|0;
            $bits = $1186;
           } else {
            while(1) {
             $1187 = $bits;
             $1188 = (($here) + 1|0);
             $1189 = HEAP8[$1188]|0;
             $1190 = $1189&255;
             $1191 = (($1190) + 7)|0;
             $1192 = ($1187>>>0)<($1191>>>0);
             if (!($1192)) {
              break;
             }
             $1193 = $have;
             $1194 = ($1193|0)==(0);
             if ($1194) {
              label = 372;
              break L13;
             }
             $1195 = $have;
             $1196 = (($1195) + -1)|0;
             $have = $1196;
             $1197 = $next;
             $1198 = (($1197) + 1|0);
             $next = $1198;
             $1199 = HEAP8[$1197]|0;
             $1200 = $1199&255;
             $1201 = $bits;
             $1202 = $1200 << $1201;
             $1203 = $hold;
             $1204 = (($1203) + ($1202))|0;
             $hold = $1204;
             $1205 = $bits;
             $1206 = (($1205) + 8)|0;
             $bits = $1206;
            }
            $1207 = (($here) + 1|0);
            $1208 = HEAP8[$1207]|0;
            $1209 = $1208&255;
            $1210 = $hold;
            $1211 = $1210 >>> $1209;
            $hold = $1211;
            $1212 = (($here) + 1|0);
            $1213 = HEAP8[$1212]|0;
            $1214 = $1213&255;
            $1215 = $bits;
            $1216 = (($1215) - ($1214))|0;
            $bits = $1216;
            $len = 0;
            $1217 = $hold;
            $1218 = $1217 & 127;
            $1219 = (11 + ($1218))|0;
            $copy = $1219;
            $1220 = $hold;
            $1221 = $1220 >>> 7;
            $hold = $1221;
            $1222 = $bits;
            $1223 = (($1222) - 7)|0;
            $bits = $1223;
           }
          }
          $1224 = $state;
          $1225 = (($1224) + 104|0);
          $1226 = HEAP32[$1225>>2]|0;
          $1227 = $copy;
          $1228 = (($1226) + ($1227))|0;
          $1229 = $state;
          $1230 = (($1229) + 96|0);
          $1231 = HEAP32[$1230>>2]|0;
          $1232 = $state;
          $1233 = (($1232) + 100|0);
          $1234 = HEAP32[$1233>>2]|0;
          $1235 = (($1231) + ($1234))|0;
          $1236 = ($1228>>>0)>($1235>>>0);
          if ($1236) {
           label = 383;
           break;
          }
          while(1) {
           $1240 = $copy;
           $1241 = (($1240) + -1)|0;
           $copy = $1241;
           $1242 = ($1240|0)!=(0);
           if (!($1242)) {
            break;
           }
           $1243 = $len;
           $1244 = $1243&65535;
           $1245 = $state;
           $1246 = (($1245) + 104|0);
           $1247 = HEAP32[$1246>>2]|0;
           $1248 = (($1247) + 1)|0;
           HEAP32[$1246>>2] = $1248;
           $1249 = $state;
           $1250 = (($1249) + 112|0);
           $1251 = (($1250) + ($1247<<1)|0);
           HEAP16[$1251>>1] = $1244;
          }
         }
        }
        if ((label|0) == 348) {
         label = 0;
         $1127 = $1;
         $1128 = (($1127) + 24|0);
         HEAP32[$1128>>2] = 8856;
         $1129 = $state;
         HEAP32[$1129>>2] = 29;
        }
        else if ((label|0) == 383) {
         label = 0;
         $1237 = $1;
         $1238 = (($1237) + 24|0);
         HEAP32[$1238>>2] = 8856;
         $1239 = $state;
         HEAP32[$1239>>2] = 29;
        }
        $1252 = $state;
        $1253 = HEAP32[$1252>>2]|0;
        $1254 = ($1253|0)==(29);
        if ($1254) {
         break;
        }
        $1255 = $state;
        $1256 = (($1255) + 112|0);
        $1257 = (($1256) + 512|0);
        $1258 = HEAP16[$1257>>1]|0;
        $1259 = $1258&65535;
        $1260 = ($1259|0)==(0);
        if ($1260) {
         $1261 = $1;
         $1262 = (($1261) + 24|0);
         HEAP32[$1262>>2] = 8888;
         $1263 = $state;
         HEAP32[$1263>>2] = 29;
         break;
        }
        $1264 = $state;
        $1265 = (($1264) + 1328|0);
        $1266 = $state;
        $1267 = (($1266) + 108|0);
        HEAP32[$1267>>2] = $1265;
        $1268 = $state;
        $1269 = (($1268) + 108|0);
        $1270 = HEAP32[$1269>>2]|0;
        $1271 = $state;
        $1272 = (($1271) + 76|0);
        HEAP32[$1272>>2] = $1270;
        $1273 = $state;
        $1274 = (($1273) + 84|0);
        HEAP32[$1274>>2] = 9;
        $1275 = $state;
        $1276 = (($1275) + 112|0);
        $1277 = $state;
        $1278 = (($1277) + 96|0);
        $1279 = HEAP32[$1278>>2]|0;
        $1280 = $state;
        $1281 = (($1280) + 108|0);
        $1282 = $state;
        $1283 = (($1282) + 84|0);
        $1284 = $state;
        $1285 = (($1284) + 752|0);
        $1286 = (_inflate_table(1,$1276,$1279,$1281,$1283,$1285)|0);
        $ret = $1286;
        $1287 = $ret;
        $1288 = ($1287|0)!=(0);
        if ($1288) {
         $1289 = $1;
         $1290 = (($1289) + 24|0);
         HEAP32[$1290>>2] = 8928;
         $1291 = $state;
         HEAP32[$1291>>2] = 29;
         break;
        }
        $1292 = $state;
        $1293 = (($1292) + 108|0);
        $1294 = HEAP32[$1293>>2]|0;
        $1295 = $state;
        $1296 = (($1295) + 80|0);
        HEAP32[$1296>>2] = $1294;
        $1297 = $state;
        $1298 = (($1297) + 88|0);
        HEAP32[$1298>>2] = 6;
        $1299 = $state;
        $1300 = (($1299) + 112|0);
        $1301 = $state;
        $1302 = (($1301) + 96|0);
        $1303 = HEAP32[$1302>>2]|0;
        $1304 = (($1300) + ($1303<<1)|0);
        $1305 = $state;
        $1306 = (($1305) + 100|0);
        $1307 = HEAP32[$1306>>2]|0;
        $1308 = $state;
        $1309 = (($1308) + 108|0);
        $1310 = $state;
        $1311 = (($1310) + 88|0);
        $1312 = $state;
        $1313 = (($1312) + 752|0);
        $1314 = (_inflate_table(2,$1304,$1307,$1309,$1311,$1313)|0);
        $ret = $1314;
        $1315 = $ret;
        $1316 = ($1315|0)!=(0);
        if ($1316) {
         $1317 = $1;
         $1318 = (($1317) + 24|0);
         HEAP32[$1318>>2] = 8960;
         $1319 = $state;
         HEAP32[$1319>>2] = 29;
         break;
        }
        $1320 = $state;
        HEAP32[$1320>>2] = 19;
        $1321 = $2;
        $1322 = ($1321|0)==(6);
        if ($1322) {
         label = 398;
         break L13;
        }
        label = 400;
       }
      } while(0);
      do {
       if ((label|0) == 106) {
        label = 0;
        $324 = $state;
        $325 = (($324) + 16|0);
        $326 = HEAP32[$325>>2]|0;
        $327 = $326 & 1024;
        $328 = ($327|0)!=(0);
        if ($328) {
         while(1) {
          $329 = $bits;
          $330 = ($329>>>0)<(16);
          if (!($330)) {
           break;
          }
          $331 = $have;
          $332 = ($331|0)==(0);
          if ($332) {
           label = 112;
           break L13;
          }
          $333 = $have;
          $334 = (($333) + -1)|0;
          $have = $334;
          $335 = $next;
          $336 = (($335) + 1|0);
          $next = $336;
          $337 = HEAP8[$335]|0;
          $338 = $337&255;
          $339 = $bits;
          $340 = $338 << $339;
          $341 = $hold;
          $342 = (($341) + ($340))|0;
          $hold = $342;
          $343 = $bits;
          $344 = (($343) + 8)|0;
          $bits = $344;
         }
         $345 = $hold;
         $346 = $state;
         $347 = (($346) + 64|0);
         HEAP32[$347>>2] = $345;
         $348 = $state;
         $349 = (($348) + 32|0);
         $350 = HEAP32[$349>>2]|0;
         $351 = ($350|0)!=(0|0);
         if ($351) {
          $352 = $hold;
          $353 = $state;
          $354 = (($353) + 32|0);
          $355 = HEAP32[$354>>2]|0;
          $356 = (($355) + 20|0);
          HEAP32[$356>>2] = $352;
         }
         $357 = $state;
         $358 = (($357) + 16|0);
         $359 = HEAP32[$358>>2]|0;
         $360 = $359 & 512;
         $361 = ($360|0)!=(0);
         if ($361) {
          $362 = $hold;
          $363 = $362&255;
          HEAP8[$hbuf] = $363;
          $364 = $hold;
          $365 = $364 >>> 8;
          $366 = $365&255;
          $367 = (($hbuf) + 1|0);
          HEAP8[$367] = $366;
          $368 = $state;
          $369 = (($368) + 24|0);
          $370 = HEAP32[$369>>2]|0;
          $371 = (_crc32($370,$hbuf,2)|0);
          $372 = $state;
          $373 = (($372) + 24|0);
          HEAP32[$373>>2] = $371;
         }
         $hold = 0;
         $bits = 0;
        } else {
         $374 = $state;
         $375 = (($374) + 32|0);
         $376 = HEAP32[$375>>2]|0;
         $377 = ($376|0)!=(0|0);
         if ($377) {
          $378 = $state;
          $379 = (($378) + 32|0);
          $380 = HEAP32[$379>>2]|0;
          $381 = (($380) + 16|0);
          HEAP32[$381>>2] = 0;
         }
        }
        $382 = $state;
        HEAP32[$382>>2] = 5;
        label = 129;
       }
       else if ((label|0) == 227) {
        label = 0;
        $753 = $state;
        $754 = (($753) + 4|0);
        $755 = HEAP32[$754>>2]|0;
        $756 = ($755|0)!=(0);
        if ($756) {
         $757 = $bits;
         $758 = $757 & 7;
         $759 = $hold;
         $760 = $759 >>> $758;
         $hold = $760;
         $761 = $bits;
         $762 = $761 & 7;
         $763 = $bits;
         $764 = (($763) - ($762))|0;
         $bits = $764;
         $765 = $state;
         HEAP32[$765>>2] = 26;
         break;
        }
        while(1) {
         $766 = $bits;
         $767 = ($766>>>0)<(3);
         if (!($767)) {
          break;
         }
         $768 = $have;
         $769 = ($768|0)==(0);
         if ($769) {
          label = 236;
          break L13;
         }
         $770 = $have;
         $771 = (($770) + -1)|0;
         $have = $771;
         $772 = $next;
         $773 = (($772) + 1|0);
         $next = $773;
         $774 = HEAP8[$772]|0;
         $775 = $774&255;
         $776 = $bits;
         $777 = $775 << $776;
         $778 = $hold;
         $779 = (($778) + ($777))|0;
         $hold = $779;
         $780 = $bits;
         $781 = (($780) + 8)|0;
         $bits = $781;
        }
        $782 = $hold;
        $783 = $782 & 1;
        $784 = $state;
        $785 = (($784) + 4|0);
        HEAP32[$785>>2] = $783;
        $786 = $hold;
        $787 = $786 >>> 1;
        $hold = $787;
        $788 = $bits;
        $789 = (($788) - 1)|0;
        $bits = $789;
        $790 = $hold;
        $791 = $790 & 3;
        if ((($791|0) == 2)) {
         $801 = $state;
         HEAP32[$801>>2] = 16;
        } else if ((($791|0) == 3)) {
         $802 = $1;
         $803 = (($802) + 24|0);
         HEAP32[$803>>2] = 8728;
         $804 = $state;
         HEAP32[$804>>2] = 29;
        } else if ((($791|0) == 0)) {
         $792 = $state;
         HEAP32[$792>>2] = 13;
        } else if ((($791|0) == 1)) {
         $793 = $state;
         _fixedtables($793);
         $794 = $state;
         HEAP32[$794>>2] = 19;
         $795 = $2;
         $796 = ($795|0)==(6);
         if ($796) {
          label = 245;
          break L13;
         }
        }
        $805 = $hold;
        $806 = $805 >>> 2;
        $hold = $806;
        $807 = $bits;
        $808 = (($807) - 2)|0;
        $bits = $808;
       }
       else if ((label|0) == 400) {
        label = 0;
        $1323 = $state;
        HEAP32[$1323>>2] = 20;
        label = 401;
       }
      } while(0);
      do {
       if ((label|0) == 129) {
        label = 0;
        $383 = $state;
        $384 = (($383) + 16|0);
        $385 = HEAP32[$384>>2]|0;
        $386 = $385 & 1024;
        $387 = ($386|0)!=(0);
        if ($387) {
         $388 = $state;
         $389 = (($388) + 64|0);
         $390 = HEAP32[$389>>2]|0;
         $copy = $390;
         $391 = $copy;
         $392 = $have;
         $393 = ($391>>>0)>($392>>>0);
         if ($393) {
          $394 = $have;
          $copy = $394;
         }
         $395 = $copy;
         $396 = ($395|0)!=(0);
         if ($396) {
          $397 = $state;
          $398 = (($397) + 32|0);
          $399 = HEAP32[$398>>2]|0;
          $400 = ($399|0)!=(0|0);
          if ($400) {
           $401 = $state;
           $402 = (($401) + 32|0);
           $403 = HEAP32[$402>>2]|0;
           $404 = (($403) + 16|0);
           $405 = HEAP32[$404>>2]|0;
           $406 = ($405|0)!=(0|0);
           if ($406) {
            $407 = $state;
            $408 = (($407) + 32|0);
            $409 = HEAP32[$408>>2]|0;
            $410 = (($409) + 20|0);
            $411 = HEAP32[$410>>2]|0;
            $412 = $state;
            $413 = (($412) + 64|0);
            $414 = HEAP32[$413>>2]|0;
            $415 = (($411) - ($414))|0;
            $len = $415;
            $416 = $state;
            $417 = (($416) + 32|0);
            $418 = HEAP32[$417>>2]|0;
            $419 = (($418) + 16|0);
            $420 = HEAP32[$419>>2]|0;
            $421 = $len;
            $422 = (($420) + ($421)|0);
            $423 = $next;
            $424 = $len;
            $425 = $copy;
            $426 = (($424) + ($425))|0;
            $427 = $state;
            $428 = (($427) + 32|0);
            $429 = HEAP32[$428>>2]|0;
            $430 = (($429) + 24|0);
            $431 = HEAP32[$430>>2]|0;
            $432 = ($426>>>0)>($431>>>0);
            if ($432) {
             $433 = $state;
             $434 = (($433) + 32|0);
             $435 = HEAP32[$434>>2]|0;
             $436 = (($435) + 24|0);
             $437 = HEAP32[$436>>2]|0;
             $438 = $len;
             $439 = (($437) - ($438))|0;
             $441 = $439;
            } else {
             $440 = $copy;
             $441 = $440;
            }
            _memcpy(($422|0),($423|0),($441|0))|0;
           }
          }
          $442 = $state;
          $443 = (($442) + 16|0);
          $444 = HEAP32[$443>>2]|0;
          $445 = $444 & 512;
          $446 = ($445|0)!=(0);
          if ($446) {
           $447 = $state;
           $448 = (($447) + 24|0);
           $449 = HEAP32[$448>>2]|0;
           $450 = $next;
           $451 = $copy;
           $452 = (_crc32($449,$450,$451)|0);
           $453 = $state;
           $454 = (($453) + 24|0);
           HEAP32[$454>>2] = $452;
          }
          $455 = $copy;
          $456 = $have;
          $457 = (($456) - ($455))|0;
          $have = $457;
          $458 = $copy;
          $459 = $next;
          $460 = (($459) + ($458)|0);
          $next = $460;
          $461 = $copy;
          $462 = $state;
          $463 = (($462) + 64|0);
          $464 = HEAP32[$463>>2]|0;
          $465 = (($464) - ($461))|0;
          HEAP32[$463>>2] = $465;
         }
         $466 = $state;
         $467 = (($466) + 64|0);
         $468 = HEAP32[$467>>2]|0;
         $469 = ($468|0)!=(0);
         if ($469) {
          label = 143;
          break L13;
         }
        }
        $470 = $state;
        $471 = (($470) + 64|0);
        HEAP32[$471>>2] = 0;
        $472 = $state;
        HEAP32[$472>>2] = 6;
        label = 146;
       }
       else if ((label|0) == 401) {
        label = 0;
        $1324 = $have;
        $1325 = ($1324>>>0)>=(6);
        if ($1325) {
         $1326 = $left;
         $1327 = ($1326>>>0)>=(258);
         if ($1327) {
          $1328 = $put;
          $1329 = $1;
          $1330 = (($1329) + 12|0);
          HEAP32[$1330>>2] = $1328;
          $1331 = $left;
          $1332 = $1;
          $1333 = (($1332) + 16|0);
          HEAP32[$1333>>2] = $1331;
          $1334 = $next;
          $1335 = $1;
          HEAP32[$1335>>2] = $1334;
          $1336 = $have;
          $1337 = $1;
          $1338 = (($1337) + 4|0);
          HEAP32[$1338>>2] = $1336;
          $1339 = $hold;
          $1340 = $state;
          $1341 = (($1340) + 56|0);
          HEAP32[$1341>>2] = $1339;
          $1342 = $bits;
          $1343 = $state;
          $1344 = (($1343) + 60|0);
          HEAP32[$1344>>2] = $1342;
          $1345 = $1;
          $1346 = $out;
          _inflate_fast($1345,$1346);
          $1347 = $1;
          $1348 = (($1347) + 12|0);
          $1349 = HEAP32[$1348>>2]|0;
          $put = $1349;
          $1350 = $1;
          $1351 = (($1350) + 16|0);
          $1352 = HEAP32[$1351>>2]|0;
          $left = $1352;
          $1353 = $1;
          $1354 = HEAP32[$1353>>2]|0;
          $next = $1354;
          $1355 = $1;
          $1356 = (($1355) + 4|0);
          $1357 = HEAP32[$1356>>2]|0;
          $have = $1357;
          $1358 = $state;
          $1359 = (($1358) + 56|0);
          $1360 = HEAP32[$1359>>2]|0;
          $hold = $1360;
          $1361 = $state;
          $1362 = (($1361) + 60|0);
          $1363 = HEAP32[$1362>>2]|0;
          $bits = $1363;
          $1364 = $state;
          $1365 = HEAP32[$1364>>2]|0;
          $1366 = ($1365|0)==(11);
          if ($1366) {
           $1367 = $state;
           $1368 = (($1367) + 7108|0);
           HEAP32[$1368>>2] = -1;
          }
          break;
         }
        }
        $1369 = $state;
        $1370 = (($1369) + 7108|0);
        HEAP32[$1370>>2] = 0;
        while(1) {
         $1371 = $hold;
         $1372 = $state;
         $1373 = (($1372) + 84|0);
         $1374 = HEAP32[$1373>>2]|0;
         $1375 = 1 << $1374;
         $1376 = (($1375) - 1)|0;
         $1377 = $1371 & $1376;
         $1378 = $state;
         $1379 = (($1378) + 76|0);
         $1380 = HEAP32[$1379>>2]|0;
         $1381 = (($1380) + ($1377<<2)|0);
         ;HEAP16[$here+0>>1]=HEAP16[$1381+0>>1]|0;HEAP16[$here+2>>1]=HEAP16[$1381+2>>1]|0;
         $1382 = (($here) + 1|0);
         $1383 = HEAP8[$1382]|0;
         $1384 = $1383&255;
         $1385 = $bits;
         $1386 = ($1384>>>0)<=($1385>>>0);
         if ($1386) {
          break;
         }
         $1387 = $have;
         $1388 = ($1387|0)==(0);
         if ($1388) {
          label = 415;
          break L13;
         }
         $1389 = $have;
         $1390 = (($1389) + -1)|0;
         $have = $1390;
         $1391 = $next;
         $1392 = (($1391) + 1|0);
         $next = $1392;
         $1393 = HEAP8[$1391]|0;
         $1394 = $1393&255;
         $1395 = $bits;
         $1396 = $1394 << $1395;
         $1397 = $hold;
         $1398 = (($1397) + ($1396))|0;
         $hold = $1398;
         $1399 = $bits;
         $1400 = (($1399) + 8)|0;
         $bits = $1400;
        }
        $1401 = HEAP8[$here]|0;
        $1402 = $1401&255;
        $1403 = ($1402|0)!=(0);
        if ($1403) {
         $1404 = HEAP8[$here]|0;
         $1405 = $1404&255;
         $1406 = $1405 & 240;
         $1407 = ($1406|0)==(0);
         if ($1407) {
          ;HEAP16[$last+0>>1]=HEAP16[$here+0>>1]|0;HEAP16[$last+2>>1]=HEAP16[$here+2>>1]|0;
          while(1) {
           $1408 = (($last) + 2|0);
           $1409 = HEAP16[$1408>>1]|0;
           $1410 = $1409&65535;
           $1411 = $hold;
           $1412 = (($last) + 1|0);
           $1413 = HEAP8[$1412]|0;
           $1414 = $1413&255;
           $1415 = HEAP8[$last]|0;
           $1416 = $1415&255;
           $1417 = (($1414) + ($1416))|0;
           $1418 = 1 << $1417;
           $1419 = (($1418) - 1)|0;
           $1420 = $1411 & $1419;
           $1421 = (($last) + 1|0);
           $1422 = HEAP8[$1421]|0;
           $1423 = $1422&255;
           $1424 = $1420 >>> $1423;
           $1425 = (($1410) + ($1424))|0;
           $1426 = $state;
           $1427 = (($1426) + 76|0);
           $1428 = HEAP32[$1427>>2]|0;
           $1429 = (($1428) + ($1425<<2)|0);
           ;HEAP16[$here+0>>1]=HEAP16[$1429+0>>1]|0;HEAP16[$here+2>>1]=HEAP16[$1429+2>>1]|0;
           $1430 = (($last) + 1|0);
           $1431 = HEAP8[$1430]|0;
           $1432 = $1431&255;
           $1433 = (($here) + 1|0);
           $1434 = HEAP8[$1433]|0;
           $1435 = $1434&255;
           $1436 = (($1432) + ($1435))|0;
           $1437 = $bits;
           $1438 = ($1436>>>0)<=($1437>>>0);
           if ($1438) {
            break;
           }
           $1439 = $have;
           $1440 = ($1439|0)==(0);
           if ($1440) {
            label = 425;
            break L13;
           }
           $1441 = $have;
           $1442 = (($1441) + -1)|0;
           $have = $1442;
           $1443 = $next;
           $1444 = (($1443) + 1|0);
           $next = $1444;
           $1445 = HEAP8[$1443]|0;
           $1446 = $1445&255;
           $1447 = $bits;
           $1448 = $1446 << $1447;
           $1449 = $hold;
           $1450 = (($1449) + ($1448))|0;
           $hold = $1450;
           $1451 = $bits;
           $1452 = (($1451) + 8)|0;
           $bits = $1452;
          }
          $1453 = (($last) + 1|0);
          $1454 = HEAP8[$1453]|0;
          $1455 = $1454&255;
          $1456 = $hold;
          $1457 = $1456 >>> $1455;
          $hold = $1457;
          $1458 = (($last) + 1|0);
          $1459 = HEAP8[$1458]|0;
          $1460 = $1459&255;
          $1461 = $bits;
          $1462 = (($1461) - ($1460))|0;
          $bits = $1462;
          $1463 = (($last) + 1|0);
          $1464 = HEAP8[$1463]|0;
          $1465 = $1464&255;
          $1466 = $state;
          $1467 = (($1466) + 7108|0);
          $1468 = HEAP32[$1467>>2]|0;
          $1469 = (($1468) + ($1465))|0;
          HEAP32[$1467>>2] = $1469;
         }
        }
        $1470 = (($here) + 1|0);
        $1471 = HEAP8[$1470]|0;
        $1472 = $1471&255;
        $1473 = $hold;
        $1474 = $1473 >>> $1472;
        $hold = $1474;
        $1475 = (($here) + 1|0);
        $1476 = HEAP8[$1475]|0;
        $1477 = $1476&255;
        $1478 = $bits;
        $1479 = (($1478) - ($1477))|0;
        $bits = $1479;
        $1480 = (($here) + 1|0);
        $1481 = HEAP8[$1480]|0;
        $1482 = $1481&255;
        $1483 = $state;
        $1484 = (($1483) + 7108|0);
        $1485 = HEAP32[$1484>>2]|0;
        $1486 = (($1485) + ($1482))|0;
        HEAP32[$1484>>2] = $1486;
        $1487 = (($here) + 2|0);
        $1488 = HEAP16[$1487>>1]|0;
        $1489 = $1488&65535;
        $1490 = $state;
        $1491 = (($1490) + 64|0);
        HEAP32[$1491>>2] = $1489;
        $1492 = HEAP8[$here]|0;
        $1493 = $1492&255;
        $1494 = ($1493|0)==(0);
        if ($1494) {
         $1495 = $state;
         HEAP32[$1495>>2] = 25;
         break;
        }
        $1496 = HEAP8[$here]|0;
        $1497 = $1496&255;
        $1498 = $1497 & 32;
        $1499 = ($1498|0)!=(0);
        if ($1499) {
         $1500 = $state;
         $1501 = (($1500) + 7108|0);
         HEAP32[$1501>>2] = -1;
         $1502 = $state;
         HEAP32[$1502>>2] = 11;
         break;
        }
        $1503 = HEAP8[$here]|0;
        $1504 = $1503&255;
        $1505 = $1504 & 64;
        $1506 = ($1505|0)!=(0);
        if ($1506) {
         $1507 = $1;
         $1508 = (($1507) + 24|0);
         HEAP32[$1508>>2] = 8520;
         $1509 = $state;
         HEAP32[$1509>>2] = 29;
         break;
        } else {
         $1510 = HEAP8[$here]|0;
         $1511 = $1510&255;
         $1512 = $1511 & 15;
         $1513 = $state;
         $1514 = (($1513) + 72|0);
         HEAP32[$1514>>2] = $1512;
         $1515 = $state;
         HEAP32[$1515>>2] = 21;
         label = 440;
         break;
        }
       }
      } while(0);
      if ((label|0) == 146) {
       label = 0;
       $473 = $state;
       $474 = (($473) + 16|0);
       $475 = HEAP32[$474>>2]|0;
       $476 = $475 & 2048;
       $477 = ($476|0)!=(0);
       if ($477) {
        $478 = $have;
        $479 = ($478|0)==(0);
        if ($479) {
         label = 148;
         break;
        }
        $copy = 0;
        while(1) {
         $480 = $copy;
         $481 = (($480) + 1)|0;
         $copy = $481;
         $482 = $next;
         $483 = (($482) + ($480)|0);
         $484 = HEAP8[$483]|0;
         $485 = $484&255;
         $len = $485;
         $486 = $state;
         $487 = (($486) + 32|0);
         $488 = HEAP32[$487>>2]|0;
         $489 = ($488|0)!=(0|0);
         if ($489) {
          $490 = $state;
          $491 = (($490) + 32|0);
          $492 = HEAP32[$491>>2]|0;
          $493 = (($492) + 28|0);
          $494 = HEAP32[$493>>2]|0;
          $495 = ($494|0)!=(0|0);
          if ($495) {
           $496 = $state;
           $497 = (($496) + 64|0);
           $498 = HEAP32[$497>>2]|0;
           $499 = $state;
           $500 = (($499) + 32|0);
           $501 = HEAP32[$500>>2]|0;
           $502 = (($501) + 32|0);
           $503 = HEAP32[$502>>2]|0;
           $504 = ($498>>>0)<($503>>>0);
           if ($504) {
            $505 = $len;
            $506 = $505&255;
            $507 = $state;
            $508 = (($507) + 64|0);
            $509 = HEAP32[$508>>2]|0;
            $510 = (($509) + 1)|0;
            HEAP32[$508>>2] = $510;
            $511 = $state;
            $512 = (($511) + 32|0);
            $513 = HEAP32[$512>>2]|0;
            $514 = (($513) + 28|0);
            $515 = HEAP32[$514>>2]|0;
            $516 = (($515) + ($509)|0);
            HEAP8[$516] = $506;
           }
          }
         }
         $517 = $len;
         $518 = ($517|0)!=(0);
         if ($518) {
          $519 = $copy;
          $520 = $have;
          $521 = ($519>>>0)<($520>>>0);
          $2132 = $521;
         } else {
          $2132 = 0;
         }
         if (!($2132)) {
          break;
         }
        }
        $522 = $state;
        $523 = (($522) + 16|0);
        $524 = HEAP32[$523>>2]|0;
        $525 = $524 & 512;
        $526 = ($525|0)!=(0);
        if ($526) {
         $527 = $state;
         $528 = (($527) + 24|0);
         $529 = HEAP32[$528>>2]|0;
         $530 = $next;
         $531 = $copy;
         $532 = (_crc32($529,$530,$531)|0);
         $533 = $state;
         $534 = (($533) + 24|0);
         HEAP32[$534>>2] = $532;
        }
        $535 = $copy;
        $536 = $have;
        $537 = (($536) - ($535))|0;
        $have = $537;
        $538 = $copy;
        $539 = $next;
        $540 = (($539) + ($538)|0);
        $next = $540;
        $541 = $len;
        $542 = ($541|0)!=(0);
        if ($542) {
         label = 161;
         break;
        }
       } else {
        $543 = $state;
        $544 = (($543) + 32|0);
        $545 = HEAP32[$544>>2]|0;
        $546 = ($545|0)!=(0|0);
        if ($546) {
         $547 = $state;
         $548 = (($547) + 32|0);
         $549 = HEAP32[$548>>2]|0;
         $550 = (($549) + 28|0);
         HEAP32[$550>>2] = 0;
        }
       }
       $551 = $state;
       $552 = (($551) + 64|0);
       HEAP32[$552>>2] = 0;
       $553 = $state;
       HEAP32[$553>>2] = 7;
       label = 167;
      }
      else if ((label|0) == 440) {
       label = 0;
       $1516 = $state;
       $1517 = (($1516) + 72|0);
       $1518 = HEAP32[$1517>>2]|0;
       $1519 = ($1518|0)!=(0);
       if ($1519) {
        while(1) {
         $1520 = $bits;
         $1521 = $state;
         $1522 = (($1521) + 72|0);
         $1523 = HEAP32[$1522>>2]|0;
         $1524 = ($1520>>>0)<($1523>>>0);
         if (!($1524)) {
          break;
         }
         $1525 = $have;
         $1526 = ($1525|0)==(0);
         if ($1526) {
          label = 446;
          break L13;
         }
         $1527 = $have;
         $1528 = (($1527) + -1)|0;
         $have = $1528;
         $1529 = $next;
         $1530 = (($1529) + 1|0);
         $next = $1530;
         $1531 = HEAP8[$1529]|0;
         $1532 = $1531&255;
         $1533 = $bits;
         $1534 = $1532 << $1533;
         $1535 = $hold;
         $1536 = (($1535) + ($1534))|0;
         $hold = $1536;
         $1537 = $bits;
         $1538 = (($1537) + 8)|0;
         $bits = $1538;
        }
        $1539 = $hold;
        $1540 = $state;
        $1541 = (($1540) + 72|0);
        $1542 = HEAP32[$1541>>2]|0;
        $1543 = 1 << $1542;
        $1544 = (($1543) - 1)|0;
        $1545 = $1539 & $1544;
        $1546 = $state;
        $1547 = (($1546) + 64|0);
        $1548 = HEAP32[$1547>>2]|0;
        $1549 = (($1548) + ($1545))|0;
        HEAP32[$1547>>2] = $1549;
        $1550 = $state;
        $1551 = (($1550) + 72|0);
        $1552 = HEAP32[$1551>>2]|0;
        $1553 = $hold;
        $1554 = $1553 >>> $1552;
        $hold = $1554;
        $1555 = $state;
        $1556 = (($1555) + 72|0);
        $1557 = HEAP32[$1556>>2]|0;
        $1558 = $bits;
        $1559 = (($1558) - ($1557))|0;
        $bits = $1559;
        $1560 = $state;
        $1561 = (($1560) + 72|0);
        $1562 = HEAP32[$1561>>2]|0;
        $1563 = $state;
        $1564 = (($1563) + 7108|0);
        $1565 = HEAP32[$1564>>2]|0;
        $1566 = (($1565) + ($1562))|0;
        HEAP32[$1564>>2] = $1566;
       }
       $1567 = $state;
       $1568 = (($1567) + 64|0);
       $1569 = HEAP32[$1568>>2]|0;
       $1570 = $state;
       $1571 = (($1570) + 7112|0);
       HEAP32[$1571>>2] = $1569;
       $1572 = $state;
       HEAP32[$1572>>2] = 22;
       label = 454;
      }
      do {
       if ((label|0) == 167) {
        label = 0;
        $554 = $state;
        $555 = (($554) + 16|0);
        $556 = HEAP32[$555>>2]|0;
        $557 = $556 & 4096;
        $558 = ($557|0)!=(0);
        if ($558) {
         $559 = $have;
         $560 = ($559|0)==(0);
         if ($560) {
          label = 169;
          break L13;
         }
         $copy = 0;
         while(1) {
          $561 = $copy;
          $562 = (($561) + 1)|0;
          $copy = $562;
          $563 = $next;
          $564 = (($563) + ($561)|0);
          $565 = HEAP8[$564]|0;
          $566 = $565&255;
          $len = $566;
          $567 = $state;
          $568 = (($567) + 32|0);
          $569 = HEAP32[$568>>2]|0;
          $570 = ($569|0)!=(0|0);
          if ($570) {
           $571 = $state;
           $572 = (($571) + 32|0);
           $573 = HEAP32[$572>>2]|0;
           $574 = (($573) + 36|0);
           $575 = HEAP32[$574>>2]|0;
           $576 = ($575|0)!=(0|0);
           if ($576) {
            $577 = $state;
            $578 = (($577) + 64|0);
            $579 = HEAP32[$578>>2]|0;
            $580 = $state;
            $581 = (($580) + 32|0);
            $582 = HEAP32[$581>>2]|0;
            $583 = (($582) + 40|0);
            $584 = HEAP32[$583>>2]|0;
            $585 = ($579>>>0)<($584>>>0);
            if ($585) {
             $586 = $len;
             $587 = $586&255;
             $588 = $state;
             $589 = (($588) + 64|0);
             $590 = HEAP32[$589>>2]|0;
             $591 = (($590) + 1)|0;
             HEAP32[$589>>2] = $591;
             $592 = $state;
             $593 = (($592) + 32|0);
             $594 = HEAP32[$593>>2]|0;
             $595 = (($594) + 36|0);
             $596 = HEAP32[$595>>2]|0;
             $597 = (($596) + ($590)|0);
             HEAP8[$597] = $587;
            }
           }
          }
          $598 = $len;
          $599 = ($598|0)!=(0);
          if ($599) {
           $600 = $copy;
           $601 = $have;
           $602 = ($600>>>0)<($601>>>0);
           $2133 = $602;
          } else {
           $2133 = 0;
          }
          if (!($2133)) {
           break;
          }
         }
         $603 = $state;
         $604 = (($603) + 16|0);
         $605 = HEAP32[$604>>2]|0;
         $606 = $605 & 512;
         $607 = ($606|0)!=(0);
         if ($607) {
          $608 = $state;
          $609 = (($608) + 24|0);
          $610 = HEAP32[$609>>2]|0;
          $611 = $next;
          $612 = $copy;
          $613 = (_crc32($610,$611,$612)|0);
          $614 = $state;
          $615 = (($614) + 24|0);
          HEAP32[$615>>2] = $613;
         }
         $616 = $copy;
         $617 = $have;
         $618 = (($617) - ($616))|0;
         $have = $618;
         $619 = $copy;
         $620 = $next;
         $621 = (($620) + ($619)|0);
         $next = $621;
         $622 = $len;
         $623 = ($622|0)!=(0);
         if ($623) {
          label = 182;
          break L13;
         }
        } else {
         $624 = $state;
         $625 = (($624) + 32|0);
         $626 = HEAP32[$625>>2]|0;
         $627 = ($626|0)!=(0|0);
         if ($627) {
          $628 = $state;
          $629 = (($628) + 32|0);
          $630 = HEAP32[$629>>2]|0;
          $631 = (($630) + 36|0);
          HEAP32[$631>>2] = 0;
         }
        }
        $632 = $state;
        HEAP32[$632>>2] = 8;
        label = 188;
       }
       else if ((label|0) == 454) {
        label = 0;
        while(1) {
         $1573 = $hold;
         $1574 = $state;
         $1575 = (($1574) + 88|0);
         $1576 = HEAP32[$1575>>2]|0;
         $1577 = 1 << $1576;
         $1578 = (($1577) - 1)|0;
         $1579 = $1573 & $1578;
         $1580 = $state;
         $1581 = (($1580) + 80|0);
         $1582 = HEAP32[$1581>>2]|0;
         $1583 = (($1582) + ($1579<<2)|0);
         ;HEAP16[$here+0>>1]=HEAP16[$1583+0>>1]|0;HEAP16[$here+2>>1]=HEAP16[$1583+2>>1]|0;
         $1584 = (($here) + 1|0);
         $1585 = HEAP8[$1584]|0;
         $1586 = $1585&255;
         $1587 = $bits;
         $1588 = ($1586>>>0)<=($1587>>>0);
         if ($1588) {
          break;
         }
         $1589 = $have;
         $1590 = ($1589|0)==(0);
         if ($1590) {
          label = 459;
          break L13;
         }
         $1591 = $have;
         $1592 = (($1591) + -1)|0;
         $have = $1592;
         $1593 = $next;
         $1594 = (($1593) + 1|0);
         $next = $1594;
         $1595 = HEAP8[$1593]|0;
         $1596 = $1595&255;
         $1597 = $bits;
         $1598 = $1596 << $1597;
         $1599 = $hold;
         $1600 = (($1599) + ($1598))|0;
         $hold = $1600;
         $1601 = $bits;
         $1602 = (($1601) + 8)|0;
         $bits = $1602;
        }
        $1603 = HEAP8[$here]|0;
        $1604 = $1603&255;
        $1605 = $1604 & 240;
        $1606 = ($1605|0)==(0);
        if ($1606) {
         ;HEAP16[$last+0>>1]=HEAP16[$here+0>>1]|0;HEAP16[$last+2>>1]=HEAP16[$here+2>>1]|0;
         while(1) {
          $1607 = (($last) + 2|0);
          $1608 = HEAP16[$1607>>1]|0;
          $1609 = $1608&65535;
          $1610 = $hold;
          $1611 = (($last) + 1|0);
          $1612 = HEAP8[$1611]|0;
          $1613 = $1612&255;
          $1614 = HEAP8[$last]|0;
          $1615 = $1614&255;
          $1616 = (($1613) + ($1615))|0;
          $1617 = 1 << $1616;
          $1618 = (($1617) - 1)|0;
          $1619 = $1610 & $1618;
          $1620 = (($last) + 1|0);
          $1621 = HEAP8[$1620]|0;
          $1622 = $1621&255;
          $1623 = $1619 >>> $1622;
          $1624 = (($1609) + ($1623))|0;
          $1625 = $state;
          $1626 = (($1625) + 80|0);
          $1627 = HEAP32[$1626>>2]|0;
          $1628 = (($1627) + ($1624<<2)|0);
          ;HEAP16[$here+0>>1]=HEAP16[$1628+0>>1]|0;HEAP16[$here+2>>1]=HEAP16[$1628+2>>1]|0;
          $1629 = (($last) + 1|0);
          $1630 = HEAP8[$1629]|0;
          $1631 = $1630&255;
          $1632 = (($here) + 1|0);
          $1633 = HEAP8[$1632]|0;
          $1634 = $1633&255;
          $1635 = (($1631) + ($1634))|0;
          $1636 = $bits;
          $1637 = ($1635>>>0)<=($1636>>>0);
          if ($1637) {
           break;
          }
          $1638 = $have;
          $1639 = ($1638|0)==(0);
          if ($1639) {
           label = 468;
           break L13;
          }
          $1640 = $have;
          $1641 = (($1640) + -1)|0;
          $have = $1641;
          $1642 = $next;
          $1643 = (($1642) + 1|0);
          $next = $1643;
          $1644 = HEAP8[$1642]|0;
          $1645 = $1644&255;
          $1646 = $bits;
          $1647 = $1645 << $1646;
          $1648 = $hold;
          $1649 = (($1648) + ($1647))|0;
          $hold = $1649;
          $1650 = $bits;
          $1651 = (($1650) + 8)|0;
          $bits = $1651;
         }
         $1652 = (($last) + 1|0);
         $1653 = HEAP8[$1652]|0;
         $1654 = $1653&255;
         $1655 = $hold;
         $1656 = $1655 >>> $1654;
         $hold = $1656;
         $1657 = (($last) + 1|0);
         $1658 = HEAP8[$1657]|0;
         $1659 = $1658&255;
         $1660 = $bits;
         $1661 = (($1660) - ($1659))|0;
         $bits = $1661;
         $1662 = (($last) + 1|0);
         $1663 = HEAP8[$1662]|0;
         $1664 = $1663&255;
         $1665 = $state;
         $1666 = (($1665) + 7108|0);
         $1667 = HEAP32[$1666>>2]|0;
         $1668 = (($1667) + ($1664))|0;
         HEAP32[$1666>>2] = $1668;
        }
        $1669 = (($here) + 1|0);
        $1670 = HEAP8[$1669]|0;
        $1671 = $1670&255;
        $1672 = $hold;
        $1673 = $1672 >>> $1671;
        $hold = $1673;
        $1674 = (($here) + 1|0);
        $1675 = HEAP8[$1674]|0;
        $1676 = $1675&255;
        $1677 = $bits;
        $1678 = (($1677) - ($1676))|0;
        $bits = $1678;
        $1679 = (($here) + 1|0);
        $1680 = HEAP8[$1679]|0;
        $1681 = $1680&255;
        $1682 = $state;
        $1683 = (($1682) + 7108|0);
        $1684 = HEAP32[$1683>>2]|0;
        $1685 = (($1684) + ($1681))|0;
        HEAP32[$1683>>2] = $1685;
        $1686 = HEAP8[$here]|0;
        $1687 = $1686&255;
        $1688 = $1687 & 64;
        $1689 = ($1688|0)!=(0);
        if ($1689) {
         $1690 = $1;
         $1691 = (($1690) + 24|0);
         HEAP32[$1691>>2] = 8496;
         $1692 = $state;
         HEAP32[$1692>>2] = 29;
         break;
        } else {
         $1693 = (($here) + 2|0);
         $1694 = HEAP16[$1693>>1]|0;
         $1695 = $1694&65535;
         $1696 = $state;
         $1697 = (($1696) + 68|0);
         HEAP32[$1697>>2] = $1695;
         $1698 = HEAP8[$here]|0;
         $1699 = $1698&255;
         $1700 = $1699 & 15;
         $1701 = $state;
         $1702 = (($1701) + 72|0);
         HEAP32[$1702>>2] = $1700;
         $1703 = $state;
         HEAP32[$1703>>2] = 23;
         label = 479;
         break;
        }
       }
      } while(0);
      do {
       if ((label|0) == 188) {
        label = 0;
        $633 = $state;
        $634 = (($633) + 16|0);
        $635 = HEAP32[$634>>2]|0;
        $636 = $635 & 512;
        $637 = ($636|0)!=(0);
        if ($637) {
         while(1) {
          $638 = $bits;
          $639 = ($638>>>0)<(16);
          if (!($639)) {
           break;
          }
          $640 = $have;
          $641 = ($640|0)==(0);
          if ($641) {
           label = 194;
           break L13;
          }
          $642 = $have;
          $643 = (($642) + -1)|0;
          $have = $643;
          $644 = $next;
          $645 = (($644) + 1|0);
          $next = $645;
          $646 = HEAP8[$644]|0;
          $647 = $646&255;
          $648 = $bits;
          $649 = $647 << $648;
          $650 = $hold;
          $651 = (($650) + ($649))|0;
          $hold = $651;
          $652 = $bits;
          $653 = (($652) + 8)|0;
          $bits = $653;
         }
         $654 = $hold;
         $655 = $state;
         $656 = (($655) + 24|0);
         $657 = HEAP32[$656>>2]|0;
         $658 = $657 & 65535;
         $659 = ($654|0)!=($658|0);
         if ($659) {
          $660 = $1;
          $661 = (($660) + 24|0);
          HEAP32[$661>>2] = 8704;
          $662 = $state;
          HEAP32[$662>>2] = 29;
          break;
         }
         $hold = 0;
         $bits = 0;
        }
        $663 = $state;
        $664 = (($663) + 32|0);
        $665 = HEAP32[$664>>2]|0;
        $666 = ($665|0)!=(0|0);
        if ($666) {
         $667 = $state;
         $668 = (($667) + 16|0);
         $669 = HEAP32[$668>>2]|0;
         $670 = $669 >> 9;
         $671 = $670 & 1;
         $672 = $state;
         $673 = (($672) + 32|0);
         $674 = HEAP32[$673>>2]|0;
         $675 = (($674) + 44|0);
         HEAP32[$675>>2] = $671;
         $676 = $state;
         $677 = (($676) + 32|0);
         $678 = HEAP32[$677>>2]|0;
         $679 = (($678) + 48|0);
         HEAP32[$679>>2] = 1;
        }
        $680 = (_crc32(0,0,0)|0);
        $681 = $state;
        $682 = (($681) + 24|0);
        HEAP32[$682>>2] = $680;
        $683 = $1;
        $684 = (($683) + 48|0);
        HEAP32[$684>>2] = $680;
        $685 = $state;
        HEAP32[$685>>2] = 11;
       }
       else if ((label|0) == 479) {
        label = 0;
        $1704 = $state;
        $1705 = (($1704) + 72|0);
        $1706 = HEAP32[$1705>>2]|0;
        $1707 = ($1706|0)!=(0);
        if ($1707) {
         while(1) {
          $1708 = $bits;
          $1709 = $state;
          $1710 = (($1709) + 72|0);
          $1711 = HEAP32[$1710>>2]|0;
          $1712 = ($1708>>>0)<($1711>>>0);
          if (!($1712)) {
           break;
          }
          $1713 = $have;
          $1714 = ($1713|0)==(0);
          if ($1714) {
           label = 485;
           break L13;
          }
          $1715 = $have;
          $1716 = (($1715) + -1)|0;
          $have = $1716;
          $1717 = $next;
          $1718 = (($1717) + 1|0);
          $next = $1718;
          $1719 = HEAP8[$1717]|0;
          $1720 = $1719&255;
          $1721 = $bits;
          $1722 = $1720 << $1721;
          $1723 = $hold;
          $1724 = (($1723) + ($1722))|0;
          $hold = $1724;
          $1725 = $bits;
          $1726 = (($1725) + 8)|0;
          $bits = $1726;
         }
         $1727 = $hold;
         $1728 = $state;
         $1729 = (($1728) + 72|0);
         $1730 = HEAP32[$1729>>2]|0;
         $1731 = 1 << $1730;
         $1732 = (($1731) - 1)|0;
         $1733 = $1727 & $1732;
         $1734 = $state;
         $1735 = (($1734) + 68|0);
         $1736 = HEAP32[$1735>>2]|0;
         $1737 = (($1736) + ($1733))|0;
         HEAP32[$1735>>2] = $1737;
         $1738 = $state;
         $1739 = (($1738) + 72|0);
         $1740 = HEAP32[$1739>>2]|0;
         $1741 = $hold;
         $1742 = $1741 >>> $1740;
         $hold = $1742;
         $1743 = $state;
         $1744 = (($1743) + 72|0);
         $1745 = HEAP32[$1744>>2]|0;
         $1746 = $bits;
         $1747 = (($1746) - ($1745))|0;
         $bits = $1747;
         $1748 = $state;
         $1749 = (($1748) + 72|0);
         $1750 = HEAP32[$1749>>2]|0;
         $1751 = $state;
         $1752 = (($1751) + 7108|0);
         $1753 = HEAP32[$1752>>2]|0;
         $1754 = (($1753) + ($1750))|0;
         HEAP32[$1752>>2] = $1754;
        }
        $1755 = $state;
        HEAP32[$1755>>2] = 24;
        label = 493;
       }
      } while(0);
      L602: do {
       if ((label|0) == 493) {
        label = 0;
        $1756 = $left;
        $1757 = ($1756|0)==(0);
        if ($1757) {
         label = 494;
         break L13;
        }
        $1758 = $out;
        $1759 = $left;
        $1760 = (($1758) - ($1759))|0;
        $copy = $1760;
        $1761 = $state;
        $1762 = (($1761) + 68|0);
        $1763 = HEAP32[$1762>>2]|0;
        $1764 = $copy;
        $1765 = ($1763>>>0)>($1764>>>0);
        if ($1765) {
         $1766 = $state;
         $1767 = (($1766) + 68|0);
         $1768 = HEAP32[$1767>>2]|0;
         $1769 = $copy;
         $1770 = (($1768) - ($1769))|0;
         $copy = $1770;
         $1771 = $copy;
         $1772 = $state;
         $1773 = (($1772) + 44|0);
         $1774 = HEAP32[$1773>>2]|0;
         $1775 = ($1771>>>0)>($1774>>>0);
         do {
          if ($1775) {
           $1776 = $state;
           $1777 = (($1776) + 7104|0);
           $1778 = HEAP32[$1777>>2]|0;
           $1779 = ($1778|0)!=(0);
           if ($1779) {
            $1780 = $1;
            $1781 = (($1780) + 24|0);
            HEAP32[$1781>>2] = 8464;
            $1782 = $state;
            HEAP32[$1782>>2] = 29;
            break L602;
           } else {
            break;
           }
          }
         } while(0);
         $1783 = $copy;
         $1784 = $state;
         $1785 = (($1784) + 48|0);
         $1786 = HEAP32[$1785>>2]|0;
         $1787 = ($1783>>>0)>($1786>>>0);
         if ($1787) {
          $1788 = $state;
          $1789 = (($1788) + 48|0);
          $1790 = HEAP32[$1789>>2]|0;
          $1791 = $copy;
          $1792 = (($1791) - ($1790))|0;
          $copy = $1792;
          $1793 = $state;
          $1794 = (($1793) + 52|0);
          $1795 = HEAP32[$1794>>2]|0;
          $1796 = $state;
          $1797 = (($1796) + 40|0);
          $1798 = HEAP32[$1797>>2]|0;
          $1799 = $copy;
          $1800 = (($1798) - ($1799))|0;
          $1801 = (($1795) + ($1800)|0);
          $from = $1801;
         } else {
          $1802 = $state;
          $1803 = (($1802) + 52|0);
          $1804 = HEAP32[$1803>>2]|0;
          $1805 = $state;
          $1806 = (($1805) + 48|0);
          $1807 = HEAP32[$1806>>2]|0;
          $1808 = $copy;
          $1809 = (($1807) - ($1808))|0;
          $1810 = (($1804) + ($1809)|0);
          $from = $1810;
         }
         $1811 = $copy;
         $1812 = $state;
         $1813 = (($1812) + 64|0);
         $1814 = HEAP32[$1813>>2]|0;
         $1815 = ($1811>>>0)>($1814>>>0);
         if ($1815) {
          $1816 = $state;
          $1817 = (($1816) + 64|0);
          $1818 = HEAP32[$1817>>2]|0;
          $copy = $1818;
         }
        } else {
         $1819 = $put;
         $1820 = $state;
         $1821 = (($1820) + 68|0);
         $1822 = HEAP32[$1821>>2]|0;
         $1823 = (0 - ($1822))|0;
         $1824 = (($1819) + ($1823)|0);
         $from = $1824;
         $1825 = $state;
         $1826 = (($1825) + 64|0);
         $1827 = HEAP32[$1826>>2]|0;
         $copy = $1827;
        }
        $1828 = $copy;
        $1829 = $left;
        $1830 = ($1828>>>0)>($1829>>>0);
        if ($1830) {
         $1831 = $left;
         $copy = $1831;
        }
        $1832 = $copy;
        $1833 = $left;
        $1834 = (($1833) - ($1832))|0;
        $left = $1834;
        $1835 = $copy;
        $1836 = $state;
        $1837 = (($1836) + 64|0);
        $1838 = HEAP32[$1837>>2]|0;
        $1839 = (($1838) - ($1835))|0;
        HEAP32[$1837>>2] = $1839;
        while(1) {
         $1840 = $from;
         $1841 = (($1840) + 1|0);
         $from = $1841;
         $1842 = HEAP8[$1840]|0;
         $1843 = $put;
         $1844 = (($1843) + 1|0);
         $put = $1844;
         HEAP8[$1843] = $1842;
         $1845 = $copy;
         $1846 = (($1845) + -1)|0;
         $copy = $1846;
         $1847 = ($1846|0)!=(0);
         if (!($1847)) {
          break;
         }
        }
        $1848 = $state;
        $1849 = (($1848) + 64|0);
        $1850 = HEAP32[$1849>>2]|0;
        $1851 = ($1850|0)==(0);
        if ($1851) {
         $1852 = $state;
         HEAP32[$1852>>2] = 20;
        }
       }
      } while(0);
     }
     if ((label|0) == 20) {
     }
     else if ((label|0) == 53) {
     }
     else if ((label|0) == 75) {
     }
     else if ((label|0) == 93) {
     }
     else if ((label|0) == 112) {
     }
     else if ((label|0) == 143) {
     }
     else if ((label|0) == 148) {
     }
     else if ((label|0) == 161) {
     }
     else if ((label|0) == 169) {
     }
     else if ((label|0) == 182) {
     }
     else if ((label|0) == 194) {
     }
     else if ((label|0) == 211) {
     }
     else if ((label|0) == 219) {
      $726 = $put;
      $727 = $1;
      $728 = (($727) + 12|0);
      HEAP32[$728>>2] = $726;
      $729 = $left;
      $730 = $1;
      $731 = (($730) + 16|0);
      HEAP32[$731>>2] = $729;
      $732 = $next;
      $733 = $1;
      HEAP32[$733>>2] = $732;
      $734 = $have;
      $735 = $1;
      $736 = (($735) + 4|0);
      HEAP32[$736>>2] = $734;
      $737 = $hold;
      $738 = $state;
      $739 = (($738) + 56|0);
      HEAP32[$739>>2] = $737;
      $740 = $bits;
      $741 = $state;
      $742 = (($741) + 60|0);
      HEAP32[$742>>2] = $740;
      $0 = 2;
      $2131 = $0;
      STACKTOP = sp;return ($2131|0);
     }
     else if ((label|0) == 225) {
     }
     else if ((label|0) == 236) {
     }
     else if ((label|0) == 245) {
      $797 = $hold;
      $798 = $797 >>> 2;
      $hold = $798;
      $799 = $bits;
      $800 = (($799) - 2)|0;
      $bits = $800;
     }
     else if ((label|0) == 261) {
     }
     else if ((label|0) == 270) {
     }
     else if ((label|0) == 279) {
     }
     else if ((label|0) == 287) {
     }
     else if ((label|0) == 308) {
     }
     else if ((label|0) == 328) {
     }
     else if ((label|0) == 341) {
     }
     else if ((label|0) == 358) {
     }
     else if ((label|0) == 372) {
     }
     else if ((label|0) == 398) {
     }
     else if ((label|0) == 415) {
     }
     else if ((label|0) == 425) {
     }
     else if ((label|0) == 446) {
     }
     else if ((label|0) == 459) {
     }
     else if ((label|0) == 468) {
     }
     else if ((label|0) == 485) {
     }
     else if ((label|0) == 494) {
     }
     else if ((label|0) == 516) {
     }
     else if ((label|0) == 524) {
     }
     else if ((label|0) == 549) {
     }
     else if ((label|0) == 555) {
      $hold = 0;
      $bits = 0;
      label = 558;
     }
     else if ((label|0) == 560) {
      $ret = -3;
     }
     else if ((label|0) == 561) {
      $0 = -4;
      $2131 = $0;
      STACKTOP = sp;return ($2131|0);
     }
     else if ((label|0) == 562) {
      label = 563;
     }
     if ((label|0) == 558) {
      $1988 = $state;
      HEAP32[$1988>>2] = 28;
      label = 559;
     }
     else if ((label|0) == 563) {
      $0 = -2;
      $2131 = $0;
      STACKTOP = sp;return ($2131|0);
     }
     if ((label|0) == 559) {
      $ret = 1;
     }
     $1989 = $put;
     $1990 = $1;
     $1991 = (($1990) + 12|0);
     HEAP32[$1991>>2] = $1989;
     $1992 = $left;
     $1993 = $1;
     $1994 = (($1993) + 16|0);
     HEAP32[$1994>>2] = $1992;
     $1995 = $next;
     $1996 = $1;
     HEAP32[$1996>>2] = $1995;
     $1997 = $have;
     $1998 = $1;
     $1999 = (($1998) + 4|0);
     HEAP32[$1999>>2] = $1997;
     $2000 = $hold;
     $2001 = $state;
     $2002 = (($2001) + 56|0);
     HEAP32[$2002>>2] = $2000;
     $2003 = $bits;
     $2004 = $state;
     $2005 = (($2004) + 60|0);
     HEAP32[$2005>>2] = $2003;
     $2006 = $state;
     $2007 = (($2006) + 40|0);
     $2008 = HEAP32[$2007>>2]|0;
     $2009 = ($2008|0)!=(0);
     if ($2009) {
      label = 572;
     } else {
      $2010 = $out;
      $2011 = $1;
      $2012 = (($2011) + 16|0);
      $2013 = HEAP32[$2012>>2]|0;
      $2014 = ($2010|0)!=($2013|0);
      if ($2014) {
       $2015 = $state;
       $2016 = HEAP32[$2015>>2]|0;
       $2017 = ($2016>>>0)<(29);
       if ($2017) {
        $2018 = $state;
        $2019 = HEAP32[$2018>>2]|0;
        $2020 = ($2019>>>0)<(26);
        if ($2020) {
         label = 572;
        } else {
         $2021 = $2;
         $2022 = ($2021|0)!=(4);
         if ($2022) {
          label = 572;
         }
        }
       }
      }
     }
     do {
      if ((label|0) == 572) {
       $2023 = $1;
       $2024 = $1;
       $2025 = (($2024) + 12|0);
       $2026 = HEAP32[$2025>>2]|0;
       $2027 = $out;
       $2028 = $1;
       $2029 = (($2028) + 16|0);
       $2030 = HEAP32[$2029>>2]|0;
       $2031 = (($2027) - ($2030))|0;
       $2032 = (_updatewindow($2023,$2026,$2031)|0);
       $2033 = ($2032|0)!=(0);
       if (!($2033)) {
        break;
       }
       $2034 = $state;
       HEAP32[$2034>>2] = 30;
       $0 = -4;
       $2131 = $0;
       STACKTOP = sp;return ($2131|0);
      }
     } while(0);
     $2035 = $1;
     $2036 = (($2035) + 4|0);
     $2037 = HEAP32[$2036>>2]|0;
     $2038 = $in;
     $2039 = (($2038) - ($2037))|0;
     $in = $2039;
     $2040 = $1;
     $2041 = (($2040) + 16|0);
     $2042 = HEAP32[$2041>>2]|0;
     $2043 = $out;
     $2044 = (($2043) - ($2042))|0;
     $out = $2044;
     $2045 = $in;
     $2046 = $1;
     $2047 = (($2046) + 8|0);
     $2048 = HEAP32[$2047>>2]|0;
     $2049 = (($2048) + ($2045))|0;
     HEAP32[$2047>>2] = $2049;
     $2050 = $out;
     $2051 = $1;
     $2052 = (($2051) + 20|0);
     $2053 = HEAP32[$2052>>2]|0;
     $2054 = (($2053) + ($2050))|0;
     HEAP32[$2052>>2] = $2054;
     $2055 = $out;
     $2056 = $state;
     $2057 = (($2056) + 28|0);
     $2058 = HEAP32[$2057>>2]|0;
     $2059 = (($2058) + ($2055))|0;
     HEAP32[$2057>>2] = $2059;
     $2060 = $state;
     $2061 = (($2060) + 8|0);
     $2062 = HEAP32[$2061>>2]|0;
     $2063 = ($2062|0)!=(0);
     if ($2063) {
      $2064 = $out;
      $2065 = ($2064|0)!=(0);
      if ($2065) {
       $2066 = $state;
       $2067 = (($2066) + 16|0);
       $2068 = HEAP32[$2067>>2]|0;
       $2069 = ($2068|0)!=(0);
       if ($2069) {
        $2070 = $state;
        $2071 = (($2070) + 24|0);
        $2072 = HEAP32[$2071>>2]|0;
        $2073 = $1;
        $2074 = (($2073) + 12|0);
        $2075 = HEAP32[$2074>>2]|0;
        $2076 = $out;
        $2077 = (0 - ($2076))|0;
        $2078 = (($2075) + ($2077)|0);
        $2079 = $out;
        $2080 = (_crc32($2072,$2078,$2079)|0);
        $2094 = $2080;
       } else {
        $2081 = $state;
        $2082 = (($2081) + 24|0);
        $2083 = HEAP32[$2082>>2]|0;
        $2084 = $1;
        $2085 = (($2084) + 12|0);
        $2086 = HEAP32[$2085>>2]|0;
        $2087 = $out;
        $2088 = (0 - ($2087))|0;
        $2089 = (($2086) + ($2088)|0);
        $2090 = $out;
        $2091 = (_adler32($2083,$2089,$2090)|0);
        $2094 = $2091;
       }
       $2092 = $state;
       $2093 = (($2092) + 24|0);
       HEAP32[$2093>>2] = $2094;
       $2095 = $1;
       $2096 = (($2095) + 48|0);
       HEAP32[$2096>>2] = $2094;
      }
     }
     $2097 = $state;
     $2098 = (($2097) + 60|0);
     $2099 = HEAP32[$2098>>2]|0;
     $2100 = $state;
     $2101 = (($2100) + 4|0);
     $2102 = HEAP32[$2101>>2]|0;
     $2103 = ($2102|0)!=(0);
     $2104 = $2103 ? 64 : 0;
     $2105 = (($2099) + ($2104))|0;
     $2106 = $state;
     $2107 = HEAP32[$2106>>2]|0;
     $2108 = ($2107|0)==(11);
     $2109 = $2108 ? 128 : 0;
     $2110 = (($2105) + ($2109))|0;
     $2111 = $state;
     $2112 = HEAP32[$2111>>2]|0;
     $2113 = ($2112|0)==(19);
     if ($2113) {
      $2118 = 1;
     } else {
      $2114 = $state;
      $2115 = HEAP32[$2114>>2]|0;
      $2116 = ($2115|0)==(14);
      $2118 = $2116;
     }
     $2117 = $2118 ? 256 : 0;
     $2119 = (($2110) + ($2117))|0;
     $2120 = $1;
     $2121 = (($2120) + 44|0);
     HEAP32[$2121>>2] = $2119;
     $2122 = $in;
     $2123 = ($2122|0)==(0);
     if ($2123) {
      $2124 = $out;
      $2125 = ($2124|0)==(0);
      if ($2125) {
       label = 586;
      } else {
       label = 585;
      }
     } else {
      label = 585;
     }
     if ((label|0) == 585) {
      $2126 = $2;
      $2127 = ($2126|0)==(4);
      if ($2127) {
       label = 586;
      }
     }
     if ((label|0) == 586) {
      $2128 = $ret;
      $2129 = ($2128|0)==(0);
      if ($2129) {
       $ret = -5;
      }
     }
     $2130 = $ret;
     $0 = $2130;
     $2131 = $0;
     STACKTOP = sp;return ($2131|0);
    }
   }
  }
 } while(0);
 $0 = -2;
 $2131 = $0;
 STACKTOP = sp;return ($2131|0);
}
function _fixedtables($state) {
 $state = $state|0;
 var $0 = 0, $1 = 0, $2 = 0, $3 = 0, $4 = 0, $5 = 0, $6 = 0, $7 = 0, $8 = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 16|0;
 $0 = $state;
 $1 = $0;
 $2 = (($1) + 76|0);
 HEAP32[$2>>2] = 9032;
 $3 = $0;
 $4 = (($3) + 84|0);
 HEAP32[$4>>2] = 9;
 $5 = $0;
 $6 = (($5) + 80|0);
 HEAP32[$6>>2] = 11080;
 $7 = $0;
 $8 = (($7) + 88|0);
 HEAP32[$8>>2] = 5;
 STACKTOP = sp;return;
}
function _updatewindow($strm,$end,$copy) {
 $strm = $strm|0;
 $end = $end|0;
 $copy = $copy|0;
 var $0 = 0, $1 = 0, $10 = 0, $100 = 0, $101 = 0, $102 = 0, $103 = 0, $104 = 0, $105 = 0, $106 = 0, $107 = 0, $108 = 0, $109 = 0, $11 = 0, $110 = 0, $111 = 0, $112 = 0, $113 = 0, $114 = 0, $115 = 0;
 var $116 = 0, $117 = 0, $118 = 0, $119 = 0, $12 = 0, $120 = 0, $121 = 0, $122 = 0, $123 = 0, $124 = 0, $125 = 0, $126 = 0, $127 = 0, $128 = 0, $129 = 0, $13 = 0, $130 = 0, $131 = 0, $132 = 0, $133 = 0;
 var $134 = 0, $135 = 0, $136 = 0, $14 = 0, $15 = 0, $16 = 0, $17 = 0, $18 = 0, $19 = 0, $2 = 0, $20 = 0, $21 = 0, $22 = 0, $23 = 0, $24 = 0, $25 = 0, $26 = 0, $27 = 0, $28 = 0, $29 = 0;
 var $3 = 0, $30 = 0, $31 = 0, $32 = 0, $33 = 0, $34 = 0, $35 = 0, $36 = 0, $37 = 0, $38 = 0, $39 = 0, $4 = 0, $40 = 0, $41 = 0, $42 = 0, $43 = 0, $44 = 0, $45 = 0, $46 = 0, $47 = 0;
 var $48 = 0, $49 = 0, $5 = 0, $50 = 0, $51 = 0, $52 = 0, $53 = 0, $54 = 0, $55 = 0, $56 = 0, $57 = 0, $58 = 0, $59 = 0, $6 = 0, $60 = 0, $61 = 0, $62 = 0, $63 = 0, $64 = 0, $65 = 0;
 var $66 = 0, $67 = 0, $68 = 0, $69 = 0, $7 = 0, $70 = 0, $71 = 0, $72 = 0, $73 = 0, $74 = 0, $75 = 0, $76 = 0, $77 = 0, $78 = 0, $79 = 0, $8 = 0, $80 = 0, $81 = 0, $82 = 0, $83 = 0;
 var $84 = 0, $85 = 0, $86 = 0, $87 = 0, $88 = 0, $89 = 0, $9 = 0, $90 = 0, $91 = 0, $92 = 0, $93 = 0, $94 = 0, $95 = 0, $96 = 0, $97 = 0, $98 = 0, $99 = 0, $dist = 0, $state = 0, label = 0;
 var sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 32|0;
 $1 = $strm;
 $2 = $end;
 $3 = $copy;
 $4 = $1;
 $5 = (($4) + 28|0);
 $6 = HEAP32[$5>>2]|0;
 $state = $6;
 $7 = $state;
 $8 = (($7) + 52|0);
 $9 = HEAP32[$8>>2]|0;
 $10 = ($9|0)==(0|0);
 do {
  if ($10) {
   $11 = $1;
   $12 = (($11) + 32|0);
   $13 = HEAP32[$12>>2]|0;
   $14 = $1;
   $15 = (($14) + 40|0);
   $16 = HEAP32[$15>>2]|0;
   $17 = $state;
   $18 = (($17) + 36|0);
   $19 = HEAP32[$18>>2]|0;
   $20 = 1 << $19;
   $21 = (FUNCTION_TABLE_iiii[$13 & 1]($16,$20,1)|0);
   $22 = $state;
   $23 = (($22) + 52|0);
   HEAP32[$23>>2] = $21;
   $24 = $state;
   $25 = (($24) + 52|0);
   $26 = HEAP32[$25>>2]|0;
   $27 = ($26|0)==(0|0);
   if (!($27)) {
    break;
   }
   $0 = 1;
   $136 = $0;
   STACKTOP = sp;return ($136|0);
  }
 } while(0);
 $28 = $state;
 $29 = (($28) + 40|0);
 $30 = HEAP32[$29>>2]|0;
 $31 = ($30|0)==(0);
 if ($31) {
  $32 = $state;
  $33 = (($32) + 36|0);
  $34 = HEAP32[$33>>2]|0;
  $35 = 1 << $34;
  $36 = $state;
  $37 = (($36) + 40|0);
  HEAP32[$37>>2] = $35;
  $38 = $state;
  $39 = (($38) + 48|0);
  HEAP32[$39>>2] = 0;
  $40 = $state;
  $41 = (($40) + 44|0);
  HEAP32[$41>>2] = 0;
 }
 $42 = $3;
 $43 = $state;
 $44 = (($43) + 40|0);
 $45 = HEAP32[$44>>2]|0;
 $46 = ($42>>>0)>=($45>>>0);
 if ($46) {
  $47 = $state;
  $48 = (($47) + 52|0);
  $49 = HEAP32[$48>>2]|0;
  $50 = $2;
  $51 = $state;
  $52 = (($51) + 40|0);
  $53 = HEAP32[$52>>2]|0;
  $54 = (0 - ($53))|0;
  $55 = (($50) + ($54)|0);
  $56 = $state;
  $57 = (($56) + 40|0);
  $58 = HEAP32[$57>>2]|0;
  _memcpy(($49|0),($55|0),($58|0))|0;
  $59 = $state;
  $60 = (($59) + 48|0);
  HEAP32[$60>>2] = 0;
  $61 = $state;
  $62 = (($61) + 40|0);
  $63 = HEAP32[$62>>2]|0;
  $64 = $state;
  $65 = (($64) + 44|0);
  HEAP32[$65>>2] = $63;
 } else {
  $66 = $state;
  $67 = (($66) + 40|0);
  $68 = HEAP32[$67>>2]|0;
  $69 = $state;
  $70 = (($69) + 48|0);
  $71 = HEAP32[$70>>2]|0;
  $72 = (($68) - ($71))|0;
  $dist = $72;
  $73 = $dist;
  $74 = $3;
  $75 = ($73>>>0)>($74>>>0);
  if ($75) {
   $76 = $3;
   $dist = $76;
  }
  $77 = $state;
  $78 = (($77) + 52|0);
  $79 = HEAP32[$78>>2]|0;
  $80 = $state;
  $81 = (($80) + 48|0);
  $82 = HEAP32[$81>>2]|0;
  $83 = (($79) + ($82)|0);
  $84 = $2;
  $85 = $3;
  $86 = (0 - ($85))|0;
  $87 = (($84) + ($86)|0);
  $88 = $dist;
  _memcpy(($83|0),($87|0),($88|0))|0;
  $89 = $dist;
  $90 = $3;
  $91 = (($90) - ($89))|0;
  $3 = $91;
  $92 = $3;
  $93 = ($92|0)!=(0);
  if ($93) {
   $94 = $state;
   $95 = (($94) + 52|0);
   $96 = HEAP32[$95>>2]|0;
   $97 = $2;
   $98 = $3;
   $99 = (0 - ($98))|0;
   $100 = (($97) + ($99)|0);
   $101 = $3;
   _memcpy(($96|0),($100|0),($101|0))|0;
   $102 = $3;
   $103 = $state;
   $104 = (($103) + 48|0);
   HEAP32[$104>>2] = $102;
   $105 = $state;
   $106 = (($105) + 40|0);
   $107 = HEAP32[$106>>2]|0;
   $108 = $state;
   $109 = (($108) + 44|0);
   HEAP32[$109>>2] = $107;
  } else {
   $110 = $dist;
   $111 = $state;
   $112 = (($111) + 48|0);
   $113 = HEAP32[$112>>2]|0;
   $114 = (($113) + ($110))|0;
   HEAP32[$112>>2] = $114;
   $115 = $state;
   $116 = (($115) + 48|0);
   $117 = HEAP32[$116>>2]|0;
   $118 = $state;
   $119 = (($118) + 40|0);
   $120 = HEAP32[$119>>2]|0;
   $121 = ($117|0)==($120|0);
   if ($121) {
    $122 = $state;
    $123 = (($122) + 48|0);
    HEAP32[$123>>2] = 0;
   }
   $124 = $state;
   $125 = (($124) + 44|0);
   $126 = HEAP32[$125>>2]|0;
   $127 = $state;
   $128 = (($127) + 40|0);
   $129 = HEAP32[$128>>2]|0;
   $130 = ($126>>>0)<($129>>>0);
   if ($130) {
    $131 = $dist;
    $132 = $state;
    $133 = (($132) + 44|0);
    $134 = HEAP32[$133>>2]|0;
    $135 = (($134) + ($131))|0;
    HEAP32[$133>>2] = $135;
   }
  }
 }
 $0 = 0;
 $136 = $0;
 STACKTOP = sp;return ($136|0);
}
function _inflateEnd($strm) {
 $strm = $strm|0;
 var $0 = 0, $1 = 0, $10 = 0, $11 = 0, $12 = 0, $13 = 0, $14 = 0, $15 = 0, $16 = 0, $17 = 0, $18 = 0, $19 = 0, $2 = 0, $20 = 0, $21 = 0, $22 = 0, $23 = 0, $24 = 0, $25 = 0, $26 = 0;
 var $27 = 0, $28 = 0, $29 = 0, $3 = 0, $30 = 0, $31 = 0, $32 = 0, $33 = 0, $34 = 0, $35 = 0, $36 = 0, $37 = 0, $38 = 0, $39 = 0, $4 = 0, $5 = 0, $6 = 0, $7 = 0, $8 = 0, $9 = 0;
 var $state = 0, label = 0, sp = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 16|0;
 $1 = $strm;
 $2 = $1;
 $3 = ($2|0)==(0|0);
 if (!($3)) {
  $4 = $1;
  $5 = (($4) + 28|0);
  $6 = HEAP32[$5>>2]|0;
  $7 = ($6|0)==(0|0);
  if (!($7)) {
   $8 = $1;
   $9 = (($8) + 36|0);
   $10 = HEAP32[$9>>2]|0;
   $11 = ($10|0)==(0|0);
   if (!($11)) {
    $12 = $1;
    $13 = (($12) + 28|0);
    $14 = HEAP32[$13>>2]|0;
    $state = $14;
    $15 = $state;
    $16 = (($15) + 52|0);
    $17 = HEAP32[$16>>2]|0;
    $18 = ($17|0)!=(0|0);
    if ($18) {
     $19 = $1;
     $20 = (($19) + 36|0);
     $21 = HEAP32[$20>>2]|0;
     $22 = $1;
     $23 = (($22) + 40|0);
     $24 = HEAP32[$23>>2]|0;
     $25 = $state;
     $26 = (($25) + 52|0);
     $27 = HEAP32[$26>>2]|0;
     FUNCTION_TABLE_vii[$21 & 1]($24,$27);
    }
    $28 = $1;
    $29 = (($28) + 36|0);
    $30 = HEAP32[$29>>2]|0;
    $31 = $1;
    $32 = (($31) + 40|0);
    $33 = HEAP32[$32>>2]|0;
    $34 = $1;
    $35 = (($34) + 28|0);
    $36 = HEAP32[$35>>2]|0;
    FUNCTION_TABLE_vii[$30 & 1]($33,$36);
    $37 = $1;
    $38 = (($37) + 28|0);
    HEAP32[$38>>2] = 0;
    $0 = 0;
    $39 = $0;
    STACKTOP = sp;return ($39|0);
   }
  }
 }
 $0 = -2;
 $39 = $0;
 STACKTOP = sp;return ($39|0);
}
function _cromon_inflate($input,$inputLength,$output,$outputLength) {
 $input = $input|0;
 $inputLength = $inputLength|0;
 $output = $output|0;
 $outputLength = $outputLength|0;
 var $0 = 0, $1 = 0, $10 = 0, $11 = 0, $12 = 0, $13 = 0, $14 = 0, $15 = 0, $16 = 0, $17 = 0, $18 = 0, $19 = 0, $2 = 0, $20 = 0, $21 = 0, $22 = 0, $23 = 0, $24 = 0, $3 = 0, $4 = 0;
 var $5 = 0, $6 = 0, $7 = 0, $8 = 0, $9 = 0, $ret = 0, $size = 0, $strm = 0, dest = 0, label = 0, sp = 0, stop = 0;
 sp = STACKTOP;
 STACKTOP = STACKTOP + 96|0;
 $strm = sp + 16|0;
 $1 = $input;
 $2 = $inputLength;
 $3 = $output;
 $4 = $outputLength;
 dest=$strm+0|0; stop=dest+56|0; do { HEAP32[dest>>2]=0|0; dest=dest+4|0; } while ((dest|0) < (stop|0));
 $size = 0;
 $ret = 0;
 $5 = $2;
 $6 = ($5>>>0)<=(0);
 if ($6) {
  $0 = -3;
  $24 = $0;
  STACKTOP = sp;return ($24|0);
 }
 $7 = $2;
 $8 = (($strm) + 4|0);
 HEAP32[$8>>2] = $7;
 $9 = $1;
 HEAP32[$strm>>2] = $9;
 $10 = $3;
 $11 = (($strm) + 12|0);
 HEAP32[$11>>2] = $10;
 $12 = $4;
 $13 = (($strm) + 16|0);
 HEAP32[$13>>2] = $12;
 (_inflateInit_($strm,8,56)|0);
 $14 = (_inflate($strm,0)|0);
 $ret = $14;
 (_inflateEnd($strm)|0);
 $15 = $ret;
 $16 = ($15|0)!=(0);
 if ($16) {
  $17 = $ret;
  $18 = ($17|0)!=(1);
  if ($18) {
   $0 = -1;
   $24 = $0;
   STACKTOP = sp;return ($24|0);
  }
 }
 $19 = $4;
 $20 = (($strm) + 16|0);
 $21 = HEAP32[$20>>2]|0;
 $22 = (($19) - ($21))|0;
 $size = $22;
 $23 = $size;
 $0 = $23;
 $24 = $0;
 STACKTOP = sp;return ($24|0);
}
function _malloc($bytes) {
 $bytes = $bytes|0;
 var $$$i = 0, $$3$i = 0, $$4$i = 0, $$pre = 0, $$pre$i = 0, $$pre$i$i = 0, $$pre$i25 = 0, $$pre$i25$i = 0, $$pre$phi$i$iZ2D = 0, $$pre$phi$i26$iZ2D = 0, $$pre$phi$i26Z2D = 0, $$pre$phi$iZ2D = 0, $$pre$phi58$i$iZ2D = 0, $$pre$phiZ2D = 0, $$pre57$i$i = 0, $$rsize$0$i = 0, $$rsize$3$i = 0, $$sum = 0, $$sum$i$i = 0, $$sum$i$i$i = 0;
 var $$sum$i14$i = 0, $$sum$i15$i = 0, $$sum$i18$i = 0, $$sum$i21$i = 0, $$sum$i2334 = 0, $$sum$i32 = 0, $$sum$i35 = 0, $$sum1 = 0, $$sum1$i = 0, $$sum1$i$i = 0, $$sum1$i16$i = 0, $$sum1$i22$i = 0, $$sum1$i24 = 0, $$sum10 = 0, $$sum10$i = 0, $$sum10$i$i = 0, $$sum10$pre$i$i = 0, $$sum107$i = 0, $$sum108$i = 0, $$sum109$i = 0;
 var $$sum11$i = 0, $$sum11$i$i = 0, $$sum11$i24$i = 0, $$sum110$i = 0, $$sum111$i = 0, $$sum1112 = 0, $$sum112$i = 0, $$sum113$i = 0, $$sum114$i = 0, $$sum115$i = 0, $$sum116$i = 0, $$sum117$i = 0, $$sum118$i = 0, $$sum119$i = 0, $$sum12$i = 0, $$sum12$i$i = 0, $$sum120$i = 0, $$sum13$i = 0, $$sum13$i$i = 0, $$sum14$i$i = 0;
 var $$sum14$pre$i = 0, $$sum15$i = 0, $$sum15$i$i = 0, $$sum16$i = 0, $$sum16$i$i = 0, $$sum17$i = 0, $$sum17$i$i = 0, $$sum18$i = 0, $$sum1819$i$i = 0, $$sum2 = 0, $$sum2$i = 0, $$sum2$i$i = 0, $$sum2$i$i$i = 0, $$sum2$i17$i = 0, $$sum2$i19$i = 0, $$sum2$i23$i = 0, $$sum2$pre$i = 0, $$sum20$i$i = 0, $$sum21$i$i = 0, $$sum22$i$i = 0;
 var $$sum23$i$i = 0, $$sum24$i$i = 0, $$sum25$i$i = 0, $$sum26$pre$i$i = 0, $$sum27$i$i = 0, $$sum28$i$i = 0, $$sum29$i$i = 0, $$sum3$i = 0, $$sum3$i$i = 0, $$sum3$i27 = 0, $$sum30$i$i = 0, $$sum3132$i$i = 0, $$sum34$i$i = 0, $$sum3536$i$i = 0, $$sum3738$i$i = 0, $$sum39$i$i = 0, $$sum4 = 0, $$sum4$i = 0, $$sum4$i28 = 0, $$sum40$i$i = 0;
 var $$sum41$i$i = 0, $$sum42$i$i = 0, $$sum5$i = 0, $$sum5$i$i = 0, $$sum56 = 0, $$sum6$i = 0, $$sum67$i$i = 0, $$sum7$i = 0, $$sum8$i = 0, $$sum8$pre = 0, $$sum9 = 0, $$sum9$i = 0, $$sum9$i$i = 0, $$tsize$1$i = 0, $$v$0$i = 0, $0 = 0, $1 = 0, $10 = 0, $100 = 0, $1000 = 0;
 var $1001 = 0, $1002 = 0, $1003 = 0, $1004 = 0, $1005 = 0, $1006 = 0, $1007 = 0, $1008 = 0, $1009 = 0, $101 = 0, $1010 = 0, $1011 = 0, $1012 = 0, $1013 = 0, $1014 = 0, $1015 = 0, $1016 = 0, $1017 = 0, $1018 = 0, $1019 = 0;
 var $102 = 0, $1020 = 0, $1021 = 0, $1022 = 0, $1023 = 0, $1024 = 0, $1025 = 0, $1026 = 0, $1027 = 0, $1028 = 0, $1029 = 0, $103 = 0, $1030 = 0, $1031 = 0, $1032 = 0, $1033 = 0, $1034 = 0, $1035 = 0, $1036 = 0, $1037 = 0;
 var $1038 = 0, $1039 = 0, $104 = 0, $1040 = 0, $1041 = 0, $1042 = 0, $1043 = 0, $1044 = 0, $1045 = 0, $1046 = 0, $1047 = 0, $1048 = 0, $1049 = 0, $105 = 0, $1050 = 0, $1051 = 0, $1052 = 0, $1053 = 0, $1054 = 0, $1055 = 0;
 var $1056 = 0, $1057 = 0, $1058 = 0, $1059 = 0, $106 = 0, $1060 = 0, $1061 = 0, $1062 = 0, $1063 = 0, $1064 = 0, $1065 = 0, $1066 = 0, $1067 = 0, $1068 = 0, $1069 = 0, $107 = 0, $1070 = 0, $1071 = 0, $1072 = 0, $1073 = 0;
 var $1074 = 0, $1075 = 0, $1076 = 0, $1077 = 0, $1078 = 0, $1079 = 0, $108 = 0, $109 = 0, $11 = 0, $110 = 0, $111 = 0, $112 = 0, $113 = 0, $114 = 0, $115 = 0, $116 = 0, $117 = 0, $118 = 0, $119 = 0, $12 = 0;
 var $120 = 0, $121 = 0, $122 = 0, $123 = 0, $124 = 0, $125 = 0, $126 = 0, $127 = 0, $128 = 0, $129 = 0, $13 = 0, $130 = 0, $131 = 0, $132 = 0, $133 = 0, $134 = 0, $135 = 0, $136 = 0, $137 = 0, $138 = 0;
 var $139 = 0, $14 = 0, $140 = 0, $141 = 0, $142 = 0, $143 = 0, $144 = 0, $145 = 0, $146 = 0, $147 = 0, $148 = 0, $149 = 0, $15 = 0, $150 = 0, $151 = 0, $152 = 0, $153 = 0, $154 = 0, $155 = 0, $156 = 0;
 var $157 = 0, $158 = 0, $159 = 0, $16 = 0, $160 = 0, $161 = 0, $162 = 0, $163 = 0, $164 = 0, $165 = 0, $166 = 0, $167 = 0, $168 = 0, $169 = 0, $17 = 0, $170 = 0, $171 = 0, $172 = 0, $173 = 0, $174 = 0;
 var $175 = 0, $176 = 0, $177 = 0, $178 = 0, $179 = 0, $18 = 0, $180 = 0, $181 = 0, $182 = 0, $183 = 0, $184 = 0, $185 = 0, $186 = 0, $187 = 0, $188 = 0, $189 = 0, $19 = 0, $190 = 0, $191 = 0, $192 = 0;
 var $193 = 0, $194 = 0, $195 = 0, $196 = 0, $197 = 0, $198 = 0, $199 = 0, $2 = 0, $20 = 0, $200 = 0, $201 = 0, $202 = 0, $203 = 0, $204 = 0, $205 = 0, $206 = 0, $207 = 0, $208 = 0, $209 = 0, $21 = 0;
 var $210 = 0, $211 = 0, $212 = 0, $213 = 0, $214 = 0, $215 = 0, $216 = 0, $217 = 0, $218 = 0, $219 = 0, $22 = 0, $220 = 0, $221 = 0, $222 = 0, $223 = 0, $224 = 0, $225 = 0, $226 = 0, $227 = 0, $228 = 0;
 var $229 = 0, $23 = 0, $230 = 0, $231 = 0, $232 = 0, $233 = 0, $234 = 0, $235 = 0, $236 = 0, $237 = 0, $238 = 0, $239 = 0, $24 = 0, $240 = 0, $241 = 0, $242 = 0, $243 = 0, $244 = 0, $245 = 0, $246 = 0;
 var $247 = 0, $248 = 0, $249 = 0, $25 = 0, $250 = 0, $251 = 0, $252 = 0, $253 = 0, $254 = 0, $255 = 0, $256 = 0, $257 = 0, $258 = 0, $259 = 0, $26 = 0, $260 = 0, $261 = 0, $262 = 0, $263 = 0, $264 = 0;
 var $265 = 0, $266 = 0, $267 = 0, $268 = 0, $269 = 0, $27 = 0, $270 = 0, $271 = 0, $272 = 0, $273 = 0, $274 = 0, $275 = 0, $276 = 0, $277 = 0, $278 = 0, $279 = 0, $28 = 0, $280 = 0, $281 = 0, $282 = 0;
 var $283 = 0, $284 = 0, $285 = 0, $286 = 0, $287 = 0, $288 = 0, $289 = 0, $29 = 0, $290 = 0, $291 = 0, $292 = 0, $293 = 0, $294 = 0, $295 = 0, $296 = 0, $297 = 0, $298 = 0, $299 = 0, $3 = 0, $30 = 0;
 var $300 = 0, $301 = 0, $302 = 0, $303 = 0, $304 = 0, $305 = 0, $306 = 0, $307 = 0, $308 = 0, $309 = 0, $31 = 0, $310 = 0, $311 = 0, $312 = 0, $313 = 0, $314 = 0, $315 = 0, $316 = 0, $317 = 0, $318 = 0;
 var $319 = 0, $32 = 0, $320 = 0, $321 = 0, $322 = 0, $323 = 0, $324 = 0, $325 = 0, $326 = 0, $327 = 0, $328 = 0, $329 = 0, $33 = 0, $330 = 0, $331 = 0, $332 = 0, $333 = 0, $334 = 0, $335 = 0, $336 = 0;
 var $337 = 0, $338 = 0, $339 = 0, $34 = 0, $340 = 0, $341 = 0, $342 = 0, $343 = 0, $344 = 0, $345 = 0, $346 = 0, $347 = 0, $348 = 0, $349 = 0, $35 = 0, $350 = 0, $351 = 0, $352 = 0, $353 = 0, $354 = 0;
 var $355 = 0, $356 = 0, $357 = 0, $358 = 0, $359 = 0, $36 = 0, $360 = 0, $361 = 0, $362 = 0, $363 = 0, $364 = 0, $365 = 0, $366 = 0, $367 = 0, $368 = 0, $369 = 0, $37 = 0, $370 = 0, $371 = 0, $372 = 0;
 var $373 = 0, $374 = 0, $375 = 0, $376 = 0, $377 = 0, $378 = 0, $379 = 0, $38 = 0, $380 = 0, $381 = 0, $382 = 0, $383 = 0, $384 = 0, $385 = 0, $386 = 0, $387 = 0, $388 = 0, $389 = 0, $39 = 0, $390 = 0;
 var $391 = 0, $392 = 0, $393 = 0, $394 = 0, $395 = 0, $396 = 0, $397 = 0, $398 = 0, $399 = 0, $4 = 0, $40 = 0, $400 = 0, $401 = 0, $402 = 0, $403 = 0, $404 = 0, $405 = 0, $406 = 0, $407 = 0, $408 = 0;
 var $409 = 0, $41 = 0, $410 = 0, $411 = 0, $412 = 0, $413 = 0, $414 = 0, $415 = 0, $416 = 0, $417 = 0, $418 = 0, $419 = 0, $42 = 0, $420 = 0, $421 = 0, $422 = 0, $423 = 0, $424 = 0, $425 = 0, $426 = 0;
 var $427 = 0, $428 = 0, $429 = 0, $43 = 0, $430 = 0, $431 = 0, $432 = 0, $433 = 0, $434 = 0, $435 = 0, $436 = 0, $437 = 0, $438 = 0, $439 = 0, $44 = 0, $440 = 0, $441 = 0, $442 = 0, $443 = 0, $444 = 0;
 var $445 = 0, $446 = 0, $447 = 0, $448 = 0, $449 = 0, $45 = 0, $450 = 0, $451 = 0, $452 = 0, $453 = 0, $454 = 0, $455 = 0, $456 = 0, $457 = 0, $458 = 0, $459 = 0, $46 = 0, $460 = 0, $461 = 0, $462 = 0;
 var $463 = 0, $464 = 0, $465 = 0, $466 = 0, $467 = 0, $468 = 0, $469 = 0, $47 = 0, $470 = 0, $471 = 0, $472 = 0, $473 = 0, $474 = 0, $475 = 0, $476 = 0, $477 = 0, $478 = 0, $479 = 0, $48 = 0, $480 = 0;
 var $481 = 0, $482 = 0, $483 = 0, $484 = 0, $485 = 0, $486 = 0, $487 = 0, $488 = 0, $489 = 0, $49 = 0, $490 = 0, $491 = 0, $492 = 0, $493 = 0, $494 = 0, $495 = 0, $496 = 0, $497 = 0, $498 = 0, $499 = 0;
 var $5 = 0, $50 = 0, $500 = 0, $501 = 0, $502 = 0, $503 = 0, $504 = 0, $505 = 0, $506 = 0, $507 = 0, $508 = 0, $509 = 0, $51 = 0, $510 = 0, $511 = 0, $512 = 0, $513 = 0, $514 = 0, $515 = 0, $516 = 0;
 var $517 = 0, $518 = 0, $519 = 0, $52 = 0, $520 = 0, $521 = 0, $522 = 0, $523 = 0, $524 = 0, $525 = 0, $526 = 0, $527 = 0, $528 = 0, $529 = 0, $53 = 0, $530 = 0, $531 = 0, $532 = 0, $533 = 0, $534 = 0;
 var $535 = 0, $536 = 0, $537 = 0, $538 = 0, $539 = 0, $54 = 0, $540 = 0, $541 = 0, $542 = 0, $543 = 0, $544 = 0, $545 = 0, $546 = 0, $547 = 0, $548 = 0, $549 = 0, $55 = 0, $550 = 0, $551 = 0, $552 = 0;
 var $553 = 0, $554 = 0, $555 = 0, $556 = 0, $557 = 0, $558 = 0, $559 = 0, $56 = 0, $560 = 0, $561 = 0, $562 = 0, $563 = 0, $564 = 0, $565 = 0, $566 = 0, $567 = 0, $568 = 0, $569 = 0, $57 = 0, $570 = 0;
 var $571 = 0, $572 = 0, $573 = 0, $574 = 0, $575 = 0, $576 = 0, $577 = 0, $578 = 0, $579 = 0, $58 = 0, $580 = 0, $581 = 0, $582 = 0, $583 = 0, $584 = 0, $585 = 0, $586 = 0, $587 = 0, $588 = 0, $589 = 0;
 var $59 = 0, $590 = 0, $591 = 0, $592 = 0, $593 = 0, $594 = 0, $595 = 0, $596 = 0, $597 = 0, $598 = 0, $599 = 0, $6 = 0, $60 = 0, $600 = 0, $601 = 0, $602 = 0, $603 = 0, $604 = 0, $605 = 0, $606 = 0;
 var $607 = 0, $608 = 0, $609 = 0, $61 = 0, $610 = 0, $611 = 0, $612 = 0, $613 = 0, $614 = 0, $615 = 0, $616 = 0, $617 = 0, $618 = 0, $619 = 0, $62 = 0, $620 = 0, $621 = 0, $622 = 0, $623 = 0, $624 = 0;
 var $625 = 0, $626 = 0, $627 = 0, $628 = 0, $629 = 0, $63 = 0, $630 = 0, $631 = 0, $632 = 0, $633 = 0, $634 = 0, $635 = 0, $636 = 0, $637 = 0, $638 = 0, $639 = 0, $64 = 0, $640 = 0, $641 = 0, $642 = 0;
 var $643 = 0, $644 = 0, $645 = 0, $646 = 0, $647 = 0, $648 = 0, $649 = 0, $65 = 0, $650 = 0, $651 = 0, $652 = 0, $653 = 0, $654 = 0, $655 = 0, $656 = 0, $657 = 0, $658 = 0, $659 = 0, $66 = 0, $660 = 0;
 var $661 = 0, $662 = 0, $663 = 0, $664 = 0, $665 = 0, $666 = 0, $667 = 0, $668 = 0, $669 = 0, $67 = 0, $670 = 0, $671 = 0, $672 = 0, $673 = 0, $674 = 0, $675 = 0, $676 = 0, $677 = 0, $678 = 0, $679 = 0;
 var $68 = 0, $680 = 0, $681 = 0, $682 = 0, $683 = 0, $684 = 0, $685 = 0, $686 = 0, $687 = 0, $688 = 0, $689 = 0, $69 = 0, $690 = 0, $691 = 0, $692 = 0, $693 = 0, $694 = 0, $695 = 0, $696 = 0, $697 = 0;
 var $698 = 0, $699 = 0, $7 = 0, $70 = 0, $700 = 0, $701 = 0, $702 = 0, $703 = 0, $704 = 0, $705 = 0, $706 = 0, $707 = 0, $708 = 0, $709 = 0, $71 = 0, $710 = 0, $711 = 0, $712 = 0, $713 = 0, $714 = 0;
 var $715 = 0, $716 = 0, $717 = 0, $718 = 0, $719 = 0, $72 = 0, $720 = 0, $721 = 0, $722 = 0, $723 = 0, $724 = 0, $725 = 0, $726 = 0, $727 = 0, $728 = 0, $729 = 0, $73 = 0, $730 = 0, $731 = 0, $732 = 0;
 var $733 = 0, $734 = 0, $735 = 0, $736 = 0, $737 = 0, $738 = 0, $739 = 0, $74 = 0, $740 = 0, $741 = 0, $742 = 0, $743 = 0, $744 = 0, $745 = 0, $746 = 0, $747 = 0, $748 = 0, $749 = 0, $75 = 0, $750 = 0;
 var $751 = 0, $752 = 0, $753 = 0, $754 = 0, $755 = 0, $756 = 0, $757 = 0, $758 = 0, $759 = 0, $76 = 0, $760 = 0, $761 = 0, $762 = 0, $763 = 0, $764 = 0, $765 = 0, $766 = 0, $767 = 0, $768 = 0, $769 = 0;
 var $77 = 0, $770 = 0, $771 = 0, $772 = 0, $773 = 0, $774 = 0, $775 = 0, $776 = 0, $777 = 0, $778 = 0, $779 = 0, $78 = 0, $780 = 0, $781 = 0, $782 = 0, $783 = 0, $784 = 0, $785 = 0, $786 = 0, $787 = 0;
 var $788 = 0, $789 = 0, $79 = 0, $790 = 0, $791 = 0, $792 = 0, $793 = 0, $794 = 0, $795 = 0, $796 = 0, $797 = 0, $798 = 0, $799 = 0, $8 = 0, $80 = 0, $800 = 0, $801 = 0, $802 = 0, $803 = 0, $804 = 0;
 var $805 = 0, $806 = 0, $807 = 0, $808 = 0, $809 = 0, $81 = 0, $810 = 0, $811 = 0, $812 = 0, $813 = 0, $814 = 0, $815 = 0, $816 = 0, $817 = 0, $818 = 0, $819 = 0, $82 = 0, $820 = 0, $821 = 0, $822 = 0;
 var $823 = 0, $824 = 0, $825 = 0, $826 = 0, $827 = 0, $828 = 0, $829 = 0, $83 = 0, $830 = 0, $831 = 0, $832 = 0, $833 = 0, $834 = 0, $835 = 0, $836 = 0, $837 = 0, $838 = 0, $839 = 0, $84 = 0, $840 = 0;
 var $841 = 0, $842 = 0, $843 = 0, $844 = 0, $845 = 0, $846 = 0, $847 = 0, $848 = 0, $849 = 0, $85 = 0, $850 = 0, $851 = 0, $852 = 0, $853 = 0, $854 = 0, $855 = 0, $856 = 0, $857 = 0, $858 = 0, $859 = 0;
 var $86 = 0, $860 = 0, $861 = 0, $862 = 0, $863 = 0, $864 = 0, $865 = 0, $866 = 0, $867 = 0, $868 = 0, $869 = 0, $87 = 0, $870 = 0, $871 = 0, $872 = 0, $873 = 0, $874 = 0, $875 = 0, $876 = 0, $877 = 0;
 var $878 = 0, $879 = 0, $88 = 0, $880 = 0, $881 = 0, $882 = 0, $883 = 0, $884 = 0, $885 = 0, $886 = 0, $887 = 0, $888 = 0, $889 = 0, $89 = 0, $890 = 0, $891 = 0, $892 = 0, $893 = 0, $894 = 0, $895 = 0;
 var $896 = 0, $897 = 0, $898 = 0, $899 = 0, $9 = 0, $90 = 0, $900 = 0, $901 = 0, $902 = 0, $903 = 0, $904 = 0, $905 = 0, $906 = 0, $907 = 0, $908 = 0, $909 = 0, $91 = 0, $910 = 0, $911 = 0, $912 = 0;
 var $913 = 0, $914 = 0, $915 = 0, $916 = 0, $917 = 0, $918 = 0, $919 = 0, $92 = 0, $920 = 0, $921 = 0, $922 = 0, $923 = 0, $924 = 0, $925 = 0, $926 = 0, $927 = 0, $928 = 0, $929 = 0, $93 = 0, $930 = 0;
 var $931 = 0, $932 = 0, $933 = 0, $934 = 0, $935 = 0, $936 = 0, $937 = 0, $938 = 0, $939 = 0, $94 = 0, $940 = 0, $941 = 0, $942 = 0, $943 = 0, $944 = 0, $945 = 0, $946 = 0, $947 = 0, $948 = 0, $949 = 0;
 var $95 = 0, $950 = 0, $951 = 0, $952 = 0, $953 = 0, $954 = 0, $955 = 0, $956 = 0, $957 = 0, $958 = 0, $959 = 0, $96 = 0, $960 = 0, $961 = 0, $962 = 0, $963 = 0, $964 = 0, $965 = 0, $966 = 0, $967 = 0;
 var $968 = 0, $969 = 0, $97 = 0, $970 = 0, $971 = 0, $972 = 0, $973 = 0, $974 = 0, $975 = 0, $976 = 0, $977 = 0, $978 = 0, $979 = 0, $98 = 0, $980 = 0, $981 = 0, $982 = 0, $983 = 0, $984 = 0, $985 = 0;
 var $986 = 0, $987 = 0, $988 = 0, $989 = 0, $99 = 0, $990 = 0, $991 = 0, $992 = 0, $993 = 0, $994 = 0, $995 = 0, $996 = 0, $997 = 0, $998 = 0, $999 = 0, $F$0$i$i = 0, $F1$0$i = 0, $F4$0 = 0, $F4$0$i$i = 0, $F5$0$i = 0;
 var $I1$0$c$i$i = 0, $I1$0$i$i = 0, $I7$0$i = 0, $I7$0$i$i = 0, $K12$025$i = 0, $K2$014$i$i = 0, $K8$052$i$i = 0, $R$0$i = 0, $R$0$i$i = 0, $R$0$i18 = 0, $R$1$i = 0, $R$1$i$i = 0, $R$1$i20 = 0, $RP$0$i = 0, $RP$0$i$i = 0, $RP$0$i17 = 0, $T$0$lcssa$i = 0, $T$0$lcssa$i$i = 0, $T$0$lcssa$i28$i = 0, $T$013$i$i = 0;
 var $T$024$i = 0, $T$051$i$i = 0, $br$0$i = 0, $cond$i = 0, $cond$i$i = 0, $cond$i21 = 0, $exitcond$i$i = 0, $i$02$i$i = 0, $idx$0$i = 0, $mem$0 = 0, $nb$0 = 0, $notlhs$i = 0, $notrhs$i = 0, $oldfirst$0$i$i = 0, $or$cond$i = 0, $or$cond$i29 = 0, $or$cond1$i = 0, $or$cond10$i = 0, $or$cond19$i = 0, $or$cond2$i = 0;
 var $or$cond49$i = 0, $or$cond5$i = 0, $or$cond6$i = 0, $or$cond8$not$i = 0, $or$cond9$i = 0, $qsize$0$i$i = 0, $rsize$0$i = 0, $rsize$0$i15 = 0, $rsize$1$i = 0, $rsize$2$i = 0, $rsize$3$lcssa$i = 0, $rsize$329$i = 0, $rst$0$i = 0, $rst$1$i = 0, $sizebits$0$i = 0, $sp$0$i$i = 0, $sp$0$i$i$i = 0, $sp$075$i = 0, $sp$168$i = 0, $ssize$0$$i = 0;
 var $ssize$0$i = 0, $ssize$1$i = 0, $ssize$2$i = 0, $t$0$i = 0, $t$0$i14 = 0, $t$1$i = 0, $t$2$ph$i = 0, $t$2$v$3$i = 0, $t$228$i = 0, $tbase$0$i = 0, $tbase$247$i = 0, $tsize$0$i = 0, $tsize$0323841$i = 0, $tsize$1$i = 0, $tsize$246$i = 0, $v$0$i = 0, $v$0$i16 = 0, $v$1$i = 0, $v$2$i = 0, $v$3$lcssa$i = 0;
 var $v$330$i = 0, label = 0, sp = 0;
 sp = STACKTOP;
 $0 = ($bytes>>>0)<(245);
 do {
  if ($0) {
   $1 = ($bytes>>>0)<(11);
   if ($1) {
    $5 = 16;
   } else {
    $2 = (($bytes) + 11)|0;
    $3 = $2 & -8;
    $5 = $3;
   }
   $4 = $5 >>> 3;
   $6 = HEAP32[11208>>2]|0;
   $7 = $6 >>> $4;
   $8 = $7 & 3;
   $9 = ($8|0)==(0);
   if (!($9)) {
    $10 = $7 & 1;
    $11 = $10 ^ 1;
    $12 = (($11) + ($4))|0;
    $13 = $12 << 1;
    $14 = ((11208 + ($13<<2)|0) + 40|0);
    $$sum10 = (($13) + 2)|0;
    $15 = ((11208 + ($$sum10<<2)|0) + 40|0);
    $16 = HEAP32[$15>>2]|0;
    $17 = (($16) + 8|0);
    $18 = HEAP32[$17>>2]|0;
    $19 = ($14|0)==($18|0);
    do {
     if ($19) {
      $20 = 1 << $12;
      $21 = $20 ^ -1;
      $22 = $6 & $21;
      HEAP32[11208>>2] = $22;
     } else {
      $23 = HEAP32[((11208 + 16|0))>>2]|0;
      $24 = ($18>>>0)<($23>>>0);
      if ($24) {
       _abort();
       // unreachable;
      }
      $25 = (($18) + 12|0);
      $26 = HEAP32[$25>>2]|0;
      $27 = ($26|0)==($16|0);
      if ($27) {
       HEAP32[$25>>2] = $14;
       HEAP32[$15>>2] = $18;
       break;
      } else {
       _abort();
       // unreachable;
      }
     }
    } while(0);
    $28 = $12 << 3;
    $29 = $28 | 3;
    $30 = (($16) + 4|0);
    HEAP32[$30>>2] = $29;
    $$sum1112 = $28 | 4;
    $31 = (($16) + ($$sum1112)|0);
    $32 = HEAP32[$31>>2]|0;
    $33 = $32 | 1;
    HEAP32[$31>>2] = $33;
    $mem$0 = $17;
    STACKTOP = sp;return ($mem$0|0);
   }
   $34 = HEAP32[((11208 + 8|0))>>2]|0;
   $35 = ($5>>>0)>($34>>>0);
   if ($35) {
    $36 = ($7|0)==(0);
    if (!($36)) {
     $37 = $7 << $4;
     $38 = 2 << $4;
     $39 = (0 - ($38))|0;
     $40 = $38 | $39;
     $41 = $37 & $40;
     $42 = (0 - ($41))|0;
     $43 = $41 & $42;
     $44 = (($43) + -1)|0;
     $45 = $44 >>> 12;
     $46 = $45 & 16;
     $47 = $44 >>> $46;
     $48 = $47 >>> 5;
     $49 = $48 & 8;
     $50 = $49 | $46;
     $51 = $47 >>> $49;
     $52 = $51 >>> 2;
     $53 = $52 & 4;
     $54 = $50 | $53;
     $55 = $51 >>> $53;
     $56 = $55 >>> 1;
     $57 = $56 & 2;
     $58 = $54 | $57;
     $59 = $55 >>> $57;
     $60 = $59 >>> 1;
     $61 = $60 & 1;
     $62 = $58 | $61;
     $63 = $59 >>> $61;
     $64 = (($62) + ($63))|0;
     $65 = $64 << 1;
     $66 = ((11208 + ($65<<2)|0) + 40|0);
     $$sum4 = (($65) + 2)|0;
     $67 = ((11208 + ($$sum4<<2)|0) + 40|0);
     $68 = HEAP32[$67>>2]|0;
     $69 = (($68) + 8|0);
     $70 = HEAP32[$69>>2]|0;
     $71 = ($66|0)==($70|0);
     do {
      if ($71) {
       $72 = 1 << $64;
       $73 = $72 ^ -1;
       $74 = $6 & $73;
       HEAP32[11208>>2] = $74;
      } else {
       $75 = HEAP32[((11208 + 16|0))>>2]|0;
       $76 = ($70>>>0)<($75>>>0);
       if ($76) {
        _abort();
        // unreachable;
       }
       $77 = (($70) + 12|0);
       $78 = HEAP32[$77>>2]|0;
       $79 = ($78|0)==($68|0);
       if ($79) {
        HEAP32[$77>>2] = $66;
        HEAP32[$67>>2] = $70;
        break;
       } else {
        _abort();
        // unreachable;
       }
      }
     } while(0);
     $80 = $64 << 3;
     $81 = (($80) - ($5))|0;
     $82 = $5 | 3;
     $83 = (($68) + 4|0);
     HEAP32[$83>>2] = $82;
     $84 = (($68) + ($5)|0);
     $85 = $81 | 1;
     $$sum56 = $5 | 4;
     $86 = (($68) + ($$sum56)|0);
     HEAP32[$86>>2] = $85;
     $87 = (($68) + ($80)|0);
     HEAP32[$87>>2] = $81;
     $88 = HEAP32[((11208 + 8|0))>>2]|0;
     $89 = ($88|0)==(0);
     if (!($89)) {
      $90 = HEAP32[((11208 + 20|0))>>2]|0;
      $91 = $88 >>> 3;
      $92 = $91 << 1;
      $93 = ((11208 + ($92<<2)|0) + 40|0);
      $94 = HEAP32[11208>>2]|0;
      $95 = 1 << $91;
      $96 = $94 & $95;
      $97 = ($96|0)==(0);
      if ($97) {
       $98 = $94 | $95;
       HEAP32[11208>>2] = $98;
       $$sum8$pre = (($92) + 2)|0;
       $$pre = ((11208 + ($$sum8$pre<<2)|0) + 40|0);
       $$pre$phiZ2D = $$pre;$F4$0 = $93;
      } else {
       $$sum9 = (($92) + 2)|0;
       $99 = ((11208 + ($$sum9<<2)|0) + 40|0);
       $100 = HEAP32[$99>>2]|0;
       $101 = HEAP32[((11208 + 16|0))>>2]|0;
       $102 = ($100>>>0)<($101>>>0);
       if ($102) {
        _abort();
        // unreachable;
       } else {
        $$pre$phiZ2D = $99;$F4$0 = $100;
       }
      }
      HEAP32[$$pre$phiZ2D>>2] = $90;
      $103 = (($F4$0) + 12|0);
      HEAP32[$103>>2] = $90;
      $104 = (($90) + 8|0);
      HEAP32[$104>>2] = $F4$0;
      $105 = (($90) + 12|0);
      HEAP32[$105>>2] = $93;
     }
     HEAP32[((11208 + 8|0))>>2] = $81;
     HEAP32[((11208 + 20|0))>>2] = $84;
     $mem$0 = $69;
     STACKTOP = sp;return ($mem$0|0);
    }
    $106 = HEAP32[((11208 + 4|0))>>2]|0;
    $107 = ($106|0)==(0);
    if ($107) {
     $nb$0 = $5;
    } else {
     $108 = (0 - ($106))|0;
     $109 = $106 & $108;
     $110 = (($109) + -1)|0;
     $111 = $110 >>> 12;
     $112 = $111 & 16;
     $113 = $110 >>> $112;
     $114 = $113 >>> 5;
     $115 = $114 & 8;
     $116 = $115 | $112;
     $117 = $113 >>> $115;
     $118 = $117 >>> 2;
     $119 = $118 & 4;
     $120 = $116 | $119;
     $121 = $117 >>> $119;
     $122 = $121 >>> 1;
     $123 = $122 & 2;
     $124 = $120 | $123;
     $125 = $121 >>> $123;
     $126 = $125 >>> 1;
     $127 = $126 & 1;
     $128 = $124 | $127;
     $129 = $125 >>> $127;
     $130 = (($128) + ($129))|0;
     $131 = ((11208 + ($130<<2)|0) + 304|0);
     $132 = HEAP32[$131>>2]|0;
     $133 = (($132) + 4|0);
     $134 = HEAP32[$133>>2]|0;
     $135 = $134 & -8;
     $136 = (($135) - ($5))|0;
     $rsize$0$i = $136;$t$0$i = $132;$v$0$i = $132;
     while(1) {
      $137 = (($t$0$i) + 16|0);
      $138 = HEAP32[$137>>2]|0;
      $139 = ($138|0)==(0|0);
      if ($139) {
       $140 = (($t$0$i) + 20|0);
       $141 = HEAP32[$140>>2]|0;
       $142 = ($141|0)==(0|0);
       if ($142) {
        break;
       } else {
        $144 = $141;
       }
      } else {
       $144 = $138;
      }
      $143 = (($144) + 4|0);
      $145 = HEAP32[$143>>2]|0;
      $146 = $145 & -8;
      $147 = (($146) - ($5))|0;
      $148 = ($147>>>0)<($rsize$0$i>>>0);
      $$rsize$0$i = $148 ? $147 : $rsize$0$i;
      $$v$0$i = $148 ? $144 : $v$0$i;
      $rsize$0$i = $$rsize$0$i;$t$0$i = $144;$v$0$i = $$v$0$i;
     }
     $149 = HEAP32[((11208 + 16|0))>>2]|0;
     $150 = ($v$0$i>>>0)<($149>>>0);
     if ($150) {
      _abort();
      // unreachable;
     }
     $151 = (($v$0$i) + ($5)|0);
     $152 = ($v$0$i>>>0)<($151>>>0);
     if (!($152)) {
      _abort();
      // unreachable;
     }
     $153 = (($v$0$i) + 24|0);
     $154 = HEAP32[$153>>2]|0;
     $155 = (($v$0$i) + 12|0);
     $156 = HEAP32[$155>>2]|0;
     $157 = ($156|0)==($v$0$i|0);
     do {
      if ($157) {
       $167 = (($v$0$i) + 20|0);
       $168 = HEAP32[$167>>2]|0;
       $169 = ($168|0)==(0|0);
       if ($169) {
        $170 = (($v$0$i) + 16|0);
        $171 = HEAP32[$170>>2]|0;
        $172 = ($171|0)==(0|0);
        if ($172) {
         $R$1$i = 0;
         break;
        } else {
         $R$0$i = $171;$RP$0$i = $170;
        }
       } else {
        $R$0$i = $168;$RP$0$i = $167;
       }
       while(1) {
        $173 = (($R$0$i) + 20|0);
        $174 = HEAP32[$173>>2]|0;
        $175 = ($174|0)==(0|0);
        if (!($175)) {
         $R$0$i = $174;$RP$0$i = $173;
         continue;
        }
        $176 = (($R$0$i) + 16|0);
        $177 = HEAP32[$176>>2]|0;
        $178 = ($177|0)==(0|0);
        if ($178) {
         break;
        } else {
         $R$0$i = $177;$RP$0$i = $176;
        }
       }
       $179 = ($RP$0$i>>>0)<($149>>>0);
       if ($179) {
        _abort();
        // unreachable;
       } else {
        HEAP32[$RP$0$i>>2] = 0;
        $R$1$i = $R$0$i;
        break;
       }
      } else {
       $158 = (($v$0$i) + 8|0);
       $159 = HEAP32[$158>>2]|0;
       $160 = ($159>>>0)<($149>>>0);
       if ($160) {
        _abort();
        // unreachable;
       }
       $161 = (($159) + 12|0);
       $162 = HEAP32[$161>>2]|0;
       $163 = ($162|0)==($v$0$i|0);
       if (!($163)) {
        _abort();
        // unreachable;
       }
       $164 = (($156) + 8|0);
       $165 = HEAP32[$164>>2]|0;
       $166 = ($165|0)==($v$0$i|0);
       if ($166) {
        HEAP32[$161>>2] = $156;
        HEAP32[$164>>2] = $159;
        $R$1$i = $156;
        break;
       } else {
        _abort();
        // unreachable;
       }
      }
     } while(0);
     $180 = ($154|0)==(0|0);
     do {
      if (!($180)) {
       $181 = (($v$0$i) + 28|0);
       $182 = HEAP32[$181>>2]|0;
       $183 = ((11208 + ($182<<2)|0) + 304|0);
       $184 = HEAP32[$183>>2]|0;
       $185 = ($v$0$i|0)==($184|0);
       if ($185) {
        HEAP32[$183>>2] = $R$1$i;
        $cond$i = ($R$1$i|0)==(0|0);
        if ($cond$i) {
         $186 = 1 << $182;
         $187 = $186 ^ -1;
         $188 = HEAP32[((11208 + 4|0))>>2]|0;
         $189 = $188 & $187;
         HEAP32[((11208 + 4|0))>>2] = $189;
         break;
        }
       } else {
        $190 = HEAP32[((11208 + 16|0))>>2]|0;
        $191 = ($154>>>0)<($190>>>0);
        if ($191) {
         _abort();
         // unreachable;
        }
        $192 = (($154) + 16|0);
        $193 = HEAP32[$192>>2]|0;
        $194 = ($193|0)==($v$0$i|0);
        if ($194) {
         HEAP32[$192>>2] = $R$1$i;
        } else {
         $195 = (($154) + 20|0);
         HEAP32[$195>>2] = $R$1$i;
        }
        $196 = ($R$1$i|0)==(0|0);
        if ($196) {
         break;
        }
       }
       $197 = HEAP32[((11208 + 16|0))>>2]|0;
       $198 = ($R$1$i>>>0)<($197>>>0);
       if ($198) {
        _abort();
        // unreachable;
       }
       $199 = (($R$1$i) + 24|0);
       HEAP32[$199>>2] = $154;
       $200 = (($v$0$i) + 16|0);
       $201 = HEAP32[$200>>2]|0;
       $202 = ($201|0)==(0|0);
       do {
        if (!($202)) {
         $203 = HEAP32[((11208 + 16|0))>>2]|0;
         $204 = ($201>>>0)<($203>>>0);
         if ($204) {
          _abort();
          // unreachable;
         } else {
          $205 = (($R$1$i) + 16|0);
          HEAP32[$205>>2] = $201;
          $206 = (($201) + 24|0);
          HEAP32[$206>>2] = $R$1$i;
          break;
         }
        }
       } while(0);
       $207 = (($v$0$i) + 20|0);
       $208 = HEAP32[$207>>2]|0;
       $209 = ($208|0)==(0|0);
       if (!($209)) {
        $210 = HEAP32[((11208 + 16|0))>>2]|0;
        $211 = ($208>>>0)<($210>>>0);
        if ($211) {
         _abort();
         // unreachable;
        } else {
         $212 = (($R$1$i) + 20|0);
         HEAP32[$212>>2] = $208;
         $213 = (($208) + 24|0);
         HEAP32[$213>>2] = $R$1$i;
         break;
        }
       }
      }
     } while(0);
     $214 = ($rsize$0$i>>>0)<(16);
     if ($214) {
      $215 = (($rsize$0$i) + ($5))|0;
      $216 = $215 | 3;
      $217 = (($v$0$i) + 4|0);
      HEAP32[$217>>2] = $216;
      $$sum4$i = (($215) + 4)|0;
      $218 = (($v$0$i) + ($$sum4$i)|0);
      $219 = HEAP32[$218>>2]|0;
      $220 = $219 | 1;
      HEAP32[$218>>2] = $220;
     } else {
      $221 = $5 | 3;
      $222 = (($v$0$i) + 4|0);
      HEAP32[$222>>2] = $221;
      $223 = $rsize$0$i | 1;
      $$sum$i35 = $5 | 4;
      $224 = (($v$0$i) + ($$sum$i35)|0);
      HEAP32[$224>>2] = $223;
      $$sum1$i = (($rsize$0$i) + ($5))|0;
      $225 = (($v$0$i) + ($$sum1$i)|0);
      HEAP32[$225>>2] = $rsize$0$i;
      $226 = HEAP32[((11208 + 8|0))>>2]|0;
      $227 = ($226|0)==(0);
      if (!($227)) {
       $228 = HEAP32[((11208 + 20|0))>>2]|0;
       $229 = $226 >>> 3;
       $230 = $229 << 1;
       $231 = ((11208 + ($230<<2)|0) + 40|0);
       $232 = HEAP32[11208>>2]|0;
       $233 = 1 << $229;
       $234 = $232 & $233;
       $235 = ($234|0)==(0);
       if ($235) {
        $236 = $232 | $233;
        HEAP32[11208>>2] = $236;
        $$sum2$pre$i = (($230) + 2)|0;
        $$pre$i = ((11208 + ($$sum2$pre$i<<2)|0) + 40|0);
        $$pre$phi$iZ2D = $$pre$i;$F1$0$i = $231;
       } else {
        $$sum3$i = (($230) + 2)|0;
        $237 = ((11208 + ($$sum3$i<<2)|0) + 40|0);
        $238 = HEAP32[$237>>2]|0;
        $239 = HEAP32[((11208 + 16|0))>>2]|0;
        $240 = ($238>>>0)<($239>>>0);
        if ($240) {
         _abort();
         // unreachable;
        } else {
         $$pre$phi$iZ2D = $237;$F1$0$i = $238;
        }
       }
       HEAP32[$$pre$phi$iZ2D>>2] = $228;
       $241 = (($F1$0$i) + 12|0);
       HEAP32[$241>>2] = $228;
       $242 = (($228) + 8|0);
       HEAP32[$242>>2] = $F1$0$i;
       $243 = (($228) + 12|0);
       HEAP32[$243>>2] = $231;
      }
      HEAP32[((11208 + 8|0))>>2] = $rsize$0$i;
      HEAP32[((11208 + 20|0))>>2] = $151;
     }
     $244 = (($v$0$i) + 8|0);
     $mem$0 = $244;
     STACKTOP = sp;return ($mem$0|0);
    }
   } else {
    $nb$0 = $5;
   }
  } else {
   $245 = ($bytes>>>0)>(4294967231);
   if ($245) {
    $nb$0 = -1;
   } else {
    $246 = (($bytes) + 11)|0;
    $247 = $246 & -8;
    $248 = HEAP32[((11208 + 4|0))>>2]|0;
    $249 = ($248|0)==(0);
    if ($249) {
     $nb$0 = $247;
    } else {
     $250 = (0 - ($247))|0;
     $251 = $246 >>> 8;
     $252 = ($251|0)==(0);
     if ($252) {
      $idx$0$i = 0;
     } else {
      $253 = ($247>>>0)>(16777215);
      if ($253) {
       $idx$0$i = 31;
      } else {
       $254 = (($251) + 1048320)|0;
       $255 = $254 >>> 16;
       $256 = $255 & 8;
       $257 = $251 << $256;
       $258 = (($257) + 520192)|0;
       $259 = $258 >>> 16;
       $260 = $259 & 4;
       $261 = $260 | $256;
       $262 = $257 << $260;
       $263 = (($262) + 245760)|0;
       $264 = $263 >>> 16;
       $265 = $264 & 2;
       $266 = $261 | $265;
       $267 = (14 - ($266))|0;
       $268 = $262 << $265;
       $269 = $268 >>> 15;
       $270 = (($267) + ($269))|0;
       $271 = $270 << 1;
       $272 = (($270) + 7)|0;
       $273 = $247 >>> $272;
       $274 = $273 & 1;
       $275 = $274 | $271;
       $idx$0$i = $275;
      }
     }
     $276 = ((11208 + ($idx$0$i<<2)|0) + 304|0);
     $277 = HEAP32[$276>>2]|0;
     $278 = ($277|0)==(0|0);
     L126: do {
      if ($278) {
       $rsize$2$i = $250;$t$1$i = 0;$v$2$i = 0;
      } else {
       $279 = ($idx$0$i|0)==(31);
       if ($279) {
        $283 = 0;
       } else {
        $280 = $idx$0$i >>> 1;
        $281 = (25 - ($280))|0;
        $283 = $281;
       }
       $282 = $247 << $283;
       $rsize$0$i15 = $250;$rst$0$i = 0;$sizebits$0$i = $282;$t$0$i14 = $277;$v$0$i16 = 0;
       while(1) {
        $284 = (($t$0$i14) + 4|0);
        $285 = HEAP32[$284>>2]|0;
        $286 = $285 & -8;
        $287 = (($286) - ($247))|0;
        $288 = ($287>>>0)<($rsize$0$i15>>>0);
        if ($288) {
         $289 = ($286|0)==($247|0);
         if ($289) {
          $rsize$2$i = $287;$t$1$i = $t$0$i14;$v$2$i = $t$0$i14;
          break L126;
         } else {
          $rsize$1$i = $287;$v$1$i = $t$0$i14;
         }
        } else {
         $rsize$1$i = $rsize$0$i15;$v$1$i = $v$0$i16;
        }
        $290 = (($t$0$i14) + 20|0);
        $291 = HEAP32[$290>>2]|0;
        $292 = $sizebits$0$i >>> 31;
        $293 = ((($t$0$i14) + ($292<<2)|0) + 16|0);
        $294 = HEAP32[$293>>2]|0;
        $295 = ($291|0)==(0|0);
        $296 = ($291|0)==($294|0);
        $or$cond$i = $295 | $296;
        $rst$1$i = $or$cond$i ? $rst$0$i : $291;
        $297 = ($294|0)==(0|0);
        $298 = $sizebits$0$i << 1;
        if ($297) {
         $rsize$2$i = $rsize$1$i;$t$1$i = $rst$1$i;$v$2$i = $v$1$i;
         break;
        } else {
         $rsize$0$i15 = $rsize$1$i;$rst$0$i = $rst$1$i;$sizebits$0$i = $298;$t$0$i14 = $294;$v$0$i16 = $v$1$i;
        }
       }
      }
     } while(0);
     $299 = ($t$1$i|0)==(0|0);
     $300 = ($v$2$i|0)==(0|0);
     $or$cond19$i = $299 & $300;
     if ($or$cond19$i) {
      $301 = 2 << $idx$0$i;
      $302 = (0 - ($301))|0;
      $303 = $301 | $302;
      $304 = $248 & $303;
      $305 = ($304|0)==(0);
      if ($305) {
       $nb$0 = $247;
       break;
      }
      $306 = (0 - ($304))|0;
      $307 = $304 & $306;
      $308 = (($307) + -1)|0;
      $309 = $308 >>> 12;
      $310 = $309 & 16;
      $311 = $308 >>> $310;
      $312 = $311 >>> 5;
      $313 = $312 & 8;
      $314 = $313 | $310;
      $315 = $311 >>> $313;
      $316 = $315 >>> 2;
      $317 = $316 & 4;
      $318 = $314 | $317;
      $319 = $315 >>> $317;
      $320 = $319 >>> 1;
      $321 = $320 & 2;
      $322 = $318 | $321;
      $323 = $319 >>> $321;
      $324 = $323 >>> 1;
      $325 = $324 & 1;
      $326 = $322 | $325;
      $327 = $323 >>> $325;
      $328 = (($326) + ($327))|0;
      $329 = ((11208 + ($328<<2)|0) + 304|0);
      $330 = HEAP32[$329>>2]|0;
      $t$2$ph$i = $330;
     } else {
      $t$2$ph$i = $t$1$i;
     }
     $331 = ($t$2$ph$i|0)==(0|0);
     if ($331) {
      $rsize$3$lcssa$i = $rsize$2$i;$v$3$lcssa$i = $v$2$i;
     } else {
      $rsize$329$i = $rsize$2$i;$t$228$i = $t$2$ph$i;$v$330$i = $v$2$i;
      while(1) {
       $332 = (($t$228$i) + 4|0);
       $333 = HEAP32[$332>>2]|0;
       $334 = $333 & -8;
       $335 = (($334) - ($247))|0;
       $336 = ($335>>>0)<($rsize$329$i>>>0);
       $$rsize$3$i = $336 ? $335 : $rsize$329$i;
       $t$2$v$3$i = $336 ? $t$228$i : $v$330$i;
       $337 = (($t$228$i) + 16|0);
       $338 = HEAP32[$337>>2]|0;
       $339 = ($338|0)==(0|0);
       if (!($339)) {
        $rsize$329$i = $$rsize$3$i;$t$228$i = $338;$v$330$i = $t$2$v$3$i;
        continue;
       }
       $340 = (($t$228$i) + 20|0);
       $341 = HEAP32[$340>>2]|0;
       $342 = ($341|0)==(0|0);
       if ($342) {
        $rsize$3$lcssa$i = $$rsize$3$i;$v$3$lcssa$i = $t$2$v$3$i;
        break;
       } else {
        $rsize$329$i = $$rsize$3$i;$t$228$i = $341;$v$330$i = $t$2$v$3$i;
       }
      }
     }
     $343 = ($v$3$lcssa$i|0)==(0|0);
     if ($343) {
      $nb$0 = $247;
     } else {
      $344 = HEAP32[((11208 + 8|0))>>2]|0;
      $345 = (($344) - ($247))|0;
      $346 = ($rsize$3$lcssa$i>>>0)<($345>>>0);
      if ($346) {
       $347 = HEAP32[((11208 + 16|0))>>2]|0;
       $348 = ($v$3$lcssa$i>>>0)<($347>>>0);
       if ($348) {
        _abort();
        // unreachable;
       }
       $349 = (($v$3$lcssa$i) + ($247)|0);
       $350 = ($v$3$lcssa$i>>>0)<($349>>>0);
       if (!($350)) {
        _abort();
        // unreachable;
       }
       $351 = (($v$3$lcssa$i) + 24|0);
       $352 = HEAP32[$351>>2]|0;
       $353 = (($v$3$lcssa$i) + 12|0);
       $354 = HEAP32[$353>>2]|0;
       $355 = ($354|0)==($v$3$lcssa$i|0);
       do {
        if ($355) {
         $365 = (($v$3$lcssa$i) + 20|0);
         $366 = HEAP32[$365>>2]|0;
         $367 = ($366|0)==(0|0);
         if ($367) {
          $368 = (($v$3$lcssa$i) + 16|0);
          $369 = HEAP32[$368>>2]|0;
          $370 = ($369|0)==(0|0);
          if ($370) {
           $R$1$i20 = 0;
           break;
          } else {
           $R$0$i18 = $369;$RP$0$i17 = $368;
          }
         } else {
          $R$0$i18 = $366;$RP$0$i17 = $365;
         }
         while(1) {
          $371 = (($R$0$i18) + 20|0);
          $372 = HEAP32[$371>>2]|0;
          $373 = ($372|0)==(0|0);
          if (!($373)) {
           $R$0$i18 = $372;$RP$0$i17 = $371;
           continue;
          }
          $374 = (($R$0$i18) + 16|0);
          $375 = HEAP32[$374>>2]|0;
          $376 = ($375|0)==(0|0);
          if ($376) {
           break;
          } else {
           $R$0$i18 = $375;$RP$0$i17 = $374;
          }
         }
         $377 = ($RP$0$i17>>>0)<($347>>>0);
         if ($377) {
          _abort();
          // unreachable;
         } else {
          HEAP32[$RP$0$i17>>2] = 0;
          $R$1$i20 = $R$0$i18;
          break;
         }
        } else {
         $356 = (($v$3$lcssa$i) + 8|0);
         $357 = HEAP32[$356>>2]|0;
         $358 = ($357>>>0)<($347>>>0);
         if ($358) {
          _abort();
          // unreachable;
         }
         $359 = (($357) + 12|0);
         $360 = HEAP32[$359>>2]|0;
         $361 = ($360|0)==($v$3$lcssa$i|0);
         if (!($361)) {
          _abort();
          // unreachable;
         }
         $362 = (($354) + 8|0);
         $363 = HEAP32[$362>>2]|0;
         $364 = ($363|0)==($v$3$lcssa$i|0);
         if ($364) {
          HEAP32[$359>>2] = $354;
          HEAP32[$362>>2] = $357;
          $R$1$i20 = $354;
          break;
         } else {
          _abort();
          // unreachable;
         }
        }
       } while(0);
       $378 = ($352|0)==(0|0);
       do {
        if (!($378)) {
         $379 = (($v$3$lcssa$i) + 28|0);
         $380 = HEAP32[$379>>2]|0;
         $381 = ((11208 + ($380<<2)|0) + 304|0);
         $382 = HEAP32[$381>>2]|0;
         $383 = ($v$3$lcssa$i|0)==($382|0);
         if ($383) {
          HEAP32[$381>>2] = $R$1$i20;
          $cond$i21 = ($R$1$i20|0)==(0|0);
          if ($cond$i21) {
           $384 = 1 << $380;
           $385 = $384 ^ -1;
           $386 = HEAP32[((11208 + 4|0))>>2]|0;
           $387 = $386 & $385;
           HEAP32[((11208 + 4|0))>>2] = $387;
           break;
          }
         } else {
          $388 = HEAP32[((11208 + 16|0))>>2]|0;
          $389 = ($352>>>0)<($388>>>0);
          if ($389) {
           _abort();
           // unreachable;
          }
          $390 = (($352) + 16|0);
          $391 = HEAP32[$390>>2]|0;
          $392 = ($391|0)==($v$3$lcssa$i|0);
          if ($392) {
           HEAP32[$390>>2] = $R$1$i20;
          } else {
           $393 = (($352) + 20|0);
           HEAP32[$393>>2] = $R$1$i20;
          }
          $394 = ($R$1$i20|0)==(0|0);
          if ($394) {
           break;
          }
         }
         $395 = HEAP32[((11208 + 16|0))>>2]|0;
         $396 = ($R$1$i20>>>0)<($395>>>0);
         if ($396) {
          _abort();
          // unreachable;
         }
         $397 = (($R$1$i20) + 24|0);
         HEAP32[$397>>2] = $352;
         $398 = (($v$3$lcssa$i) + 16|0);
         $399 = HEAP32[$398>>2]|0;
         $400 = ($399|0)==(0|0);
         do {
          if (!($400)) {
           $401 = HEAP32[((11208 + 16|0))>>2]|0;
           $402 = ($399>>>0)<($401>>>0);
           if ($402) {
            _abort();
            // unreachable;
           } else {
            $403 = (($R$1$i20) + 16|0);
            HEAP32[$403>>2] = $399;
            $404 = (($399) + 24|0);
            HEAP32[$404>>2] = $R$1$i20;
            break;
           }
          }
         } while(0);
         $405 = (($v$3$lcssa$i) + 20|0);
         $406 = HEAP32[$405>>2]|0;
         $407 = ($406|0)==(0|0);
         if (!($407)) {
          $408 = HEAP32[((11208 + 16|0))>>2]|0;
          $409 = ($406>>>0)<($408>>>0);
          if ($409) {
           _abort();
           // unreachable;
          } else {
           $410 = (($R$1$i20) + 20|0);
           HEAP32[$410>>2] = $406;
           $411 = (($406) + 24|0);
           HEAP32[$411>>2] = $R$1$i20;
           break;
          }
         }
        }
       } while(0);
       $412 = ($rsize$3$lcssa$i>>>0)<(16);
       L204: do {
        if ($412) {
         $413 = (($rsize$3$lcssa$i) + ($247))|0;
         $414 = $413 | 3;
         $415 = (($v$3$lcssa$i) + 4|0);
         HEAP32[$415>>2] = $414;
         $$sum18$i = (($413) + 4)|0;
         $416 = (($v$3$lcssa$i) + ($$sum18$i)|0);
         $417 = HEAP32[$416>>2]|0;
         $418 = $417 | 1;
         HEAP32[$416>>2] = $418;
        } else {
         $419 = $247 | 3;
         $420 = (($v$3$lcssa$i) + 4|0);
         HEAP32[$420>>2] = $419;
         $421 = $rsize$3$lcssa$i | 1;
         $$sum$i2334 = $247 | 4;
         $422 = (($v$3$lcssa$i) + ($$sum$i2334)|0);
         HEAP32[$422>>2] = $421;
         $$sum1$i24 = (($rsize$3$lcssa$i) + ($247))|0;
         $423 = (($v$3$lcssa$i) + ($$sum1$i24)|0);
         HEAP32[$423>>2] = $rsize$3$lcssa$i;
         $424 = $rsize$3$lcssa$i >>> 3;
         $425 = ($rsize$3$lcssa$i>>>0)<(256);
         if ($425) {
          $426 = $424 << 1;
          $427 = ((11208 + ($426<<2)|0) + 40|0);
          $428 = HEAP32[11208>>2]|0;
          $429 = 1 << $424;
          $430 = $428 & $429;
          $431 = ($430|0)==(0);
          if ($431) {
           $432 = $428 | $429;
           HEAP32[11208>>2] = $432;
           $$sum14$pre$i = (($426) + 2)|0;
           $$pre$i25 = ((11208 + ($$sum14$pre$i<<2)|0) + 40|0);
           $$pre$phi$i26Z2D = $$pre$i25;$F5$0$i = $427;
          } else {
           $$sum17$i = (($426) + 2)|0;
           $433 = ((11208 + ($$sum17$i<<2)|0) + 40|0);
           $434 = HEAP32[$433>>2]|0;
           $435 = HEAP32[((11208 + 16|0))>>2]|0;
           $436 = ($434>>>0)<($435>>>0);
           if ($436) {
            _abort();
            // unreachable;
           } else {
            $$pre$phi$i26Z2D = $433;$F5$0$i = $434;
           }
          }
          HEAP32[$$pre$phi$i26Z2D>>2] = $349;
          $437 = (($F5$0$i) + 12|0);
          HEAP32[$437>>2] = $349;
          $$sum15$i = (($247) + 8)|0;
          $438 = (($v$3$lcssa$i) + ($$sum15$i)|0);
          HEAP32[$438>>2] = $F5$0$i;
          $$sum16$i = (($247) + 12)|0;
          $439 = (($v$3$lcssa$i) + ($$sum16$i)|0);
          HEAP32[$439>>2] = $427;
          break;
         }
         $440 = $rsize$3$lcssa$i >>> 8;
         $441 = ($440|0)==(0);
         if ($441) {
          $I7$0$i = 0;
         } else {
          $442 = ($rsize$3$lcssa$i>>>0)>(16777215);
          if ($442) {
           $I7$0$i = 31;
          } else {
           $443 = (($440) + 1048320)|0;
           $444 = $443 >>> 16;
           $445 = $444 & 8;
           $446 = $440 << $445;
           $447 = (($446) + 520192)|0;
           $448 = $447 >>> 16;
           $449 = $448 & 4;
           $450 = $449 | $445;
           $451 = $446 << $449;
           $452 = (($451) + 245760)|0;
           $453 = $452 >>> 16;
           $454 = $453 & 2;
           $455 = $450 | $454;
           $456 = (14 - ($455))|0;
           $457 = $451 << $454;
           $458 = $457 >>> 15;
           $459 = (($456) + ($458))|0;
           $460 = $459 << 1;
           $461 = (($459) + 7)|0;
           $462 = $rsize$3$lcssa$i >>> $461;
           $463 = $462 & 1;
           $464 = $463 | $460;
           $I7$0$i = $464;
          }
         }
         $465 = ((11208 + ($I7$0$i<<2)|0) + 304|0);
         $$sum2$i = (($247) + 28)|0;
         $466 = (($v$3$lcssa$i) + ($$sum2$i)|0);
         HEAP32[$466>>2] = $I7$0$i;
         $$sum3$i27 = (($247) + 16)|0;
         $467 = (($v$3$lcssa$i) + ($$sum3$i27)|0);
         $$sum4$i28 = (($247) + 20)|0;
         $468 = (($v$3$lcssa$i) + ($$sum4$i28)|0);
         HEAP32[$468>>2] = 0;
         HEAP32[$467>>2] = 0;
         $469 = HEAP32[((11208 + 4|0))>>2]|0;
         $470 = 1 << $I7$0$i;
         $471 = $469 & $470;
         $472 = ($471|0)==(0);
         if ($472) {
          $473 = $469 | $470;
          HEAP32[((11208 + 4|0))>>2] = $473;
          HEAP32[$465>>2] = $349;
          $$sum5$i = (($247) + 24)|0;
          $474 = (($v$3$lcssa$i) + ($$sum5$i)|0);
          HEAP32[$474>>2] = $465;
          $$sum6$i = (($247) + 12)|0;
          $475 = (($v$3$lcssa$i) + ($$sum6$i)|0);
          HEAP32[$475>>2] = $349;
          $$sum7$i = (($247) + 8)|0;
          $476 = (($v$3$lcssa$i) + ($$sum7$i)|0);
          HEAP32[$476>>2] = $349;
          break;
         }
         $477 = HEAP32[$465>>2]|0;
         $478 = ($I7$0$i|0)==(31);
         if ($478) {
          $486 = 0;
         } else {
          $479 = $I7$0$i >>> 1;
          $480 = (25 - ($479))|0;
          $486 = $480;
         }
         $481 = (($477) + 4|0);
         $482 = HEAP32[$481>>2]|0;
         $483 = $482 & -8;
         $484 = ($483|0)==($rsize$3$lcssa$i|0);
         L224: do {
          if ($484) {
           $T$0$lcssa$i = $477;
          } else {
           $485 = $rsize$3$lcssa$i << $486;
           $K12$025$i = $485;$T$024$i = $477;
           while(1) {
            $493 = $K12$025$i >>> 31;
            $494 = ((($T$024$i) + ($493<<2)|0) + 16|0);
            $489 = HEAP32[$494>>2]|0;
            $495 = ($489|0)==(0|0);
            if ($495) {
             break;
            }
            $487 = $K12$025$i << 1;
            $488 = (($489) + 4|0);
            $490 = HEAP32[$488>>2]|0;
            $491 = $490 & -8;
            $492 = ($491|0)==($rsize$3$lcssa$i|0);
            if ($492) {
             $T$0$lcssa$i = $489;
             break L224;
            } else {
             $K12$025$i = $487;$T$024$i = $489;
            }
           }
           $496 = HEAP32[((11208 + 16|0))>>2]|0;
           $497 = ($494>>>0)<($496>>>0);
           if ($497) {
            _abort();
            // unreachable;
           } else {
            HEAP32[$494>>2] = $349;
            $$sum11$i = (($247) + 24)|0;
            $498 = (($v$3$lcssa$i) + ($$sum11$i)|0);
            HEAP32[$498>>2] = $T$024$i;
            $$sum12$i = (($247) + 12)|0;
            $499 = (($v$3$lcssa$i) + ($$sum12$i)|0);
            HEAP32[$499>>2] = $349;
            $$sum13$i = (($247) + 8)|0;
            $500 = (($v$3$lcssa$i) + ($$sum13$i)|0);
            HEAP32[$500>>2] = $349;
            break L204;
           }
          }
         } while(0);
         $501 = (($T$0$lcssa$i) + 8|0);
         $502 = HEAP32[$501>>2]|0;
         $503 = HEAP32[((11208 + 16|0))>>2]|0;
         $504 = ($T$0$lcssa$i>>>0)<($503>>>0);
         if ($504) {
          _abort();
          // unreachable;
         }
         $505 = ($502>>>0)<($503>>>0);
         if ($505) {
          _abort();
          // unreachable;
         } else {
          $506 = (($502) + 12|0);
          HEAP32[$506>>2] = $349;
          HEAP32[$501>>2] = $349;
          $$sum8$i = (($247) + 8)|0;
          $507 = (($v$3$lcssa$i) + ($$sum8$i)|0);
          HEAP32[$507>>2] = $502;
          $$sum9$i = (($247) + 12)|0;
          $508 = (($v$3$lcssa$i) + ($$sum9$i)|0);
          HEAP32[$508>>2] = $T$0$lcssa$i;
          $$sum10$i = (($247) + 24)|0;
          $509 = (($v$3$lcssa$i) + ($$sum10$i)|0);
          HEAP32[$509>>2] = 0;
          break;
         }
        }
       } while(0);
       $510 = (($v$3$lcssa$i) + 8|0);
       $mem$0 = $510;
       STACKTOP = sp;return ($mem$0|0);
      } else {
       $nb$0 = $247;
      }
     }
    }
   }
  }
 } while(0);
 $511 = HEAP32[((11208 + 8|0))>>2]|0;
 $512 = ($nb$0>>>0)>($511>>>0);
 if (!($512)) {
  $513 = (($511) - ($nb$0))|0;
  $514 = HEAP32[((11208 + 20|0))>>2]|0;
  $515 = ($513>>>0)>(15);
  if ($515) {
   $516 = (($514) + ($nb$0)|0);
   HEAP32[((11208 + 20|0))>>2] = $516;
   HEAP32[((11208 + 8|0))>>2] = $513;
   $517 = $513 | 1;
   $$sum2 = (($nb$0) + 4)|0;
   $518 = (($514) + ($$sum2)|0);
   HEAP32[$518>>2] = $517;
   $519 = (($514) + ($511)|0);
   HEAP32[$519>>2] = $513;
   $520 = $nb$0 | 3;
   $521 = (($514) + 4|0);
   HEAP32[$521>>2] = $520;
  } else {
   HEAP32[((11208 + 8|0))>>2] = 0;
   HEAP32[((11208 + 20|0))>>2] = 0;
   $522 = $511 | 3;
   $523 = (($514) + 4|0);
   HEAP32[$523>>2] = $522;
   $$sum1 = (($511) + 4)|0;
   $524 = (($514) + ($$sum1)|0);
   $525 = HEAP32[$524>>2]|0;
   $526 = $525 | 1;
   HEAP32[$524>>2] = $526;
  }
  $527 = (($514) + 8|0);
  $mem$0 = $527;
  STACKTOP = sp;return ($mem$0|0);
 }
 $528 = HEAP32[((11208 + 12|0))>>2]|0;
 $529 = ($nb$0>>>0)<($528>>>0);
 if ($529) {
  $530 = (($528) - ($nb$0))|0;
  HEAP32[((11208 + 12|0))>>2] = $530;
  $531 = HEAP32[((11208 + 24|0))>>2]|0;
  $532 = (($531) + ($nb$0)|0);
  HEAP32[((11208 + 24|0))>>2] = $532;
  $533 = $530 | 1;
  $$sum = (($nb$0) + 4)|0;
  $534 = (($531) + ($$sum)|0);
  HEAP32[$534>>2] = $533;
  $535 = $nb$0 | 3;
  $536 = (($531) + 4|0);
  HEAP32[$536>>2] = $535;
  $537 = (($531) + 8|0);
  $mem$0 = $537;
  STACKTOP = sp;return ($mem$0|0);
 }
 $538 = HEAP32[11680>>2]|0;
 $539 = ($538|0)==(0);
 do {
  if ($539) {
   $540 = (_sysconf(30)|0);
   $541 = (($540) + -1)|0;
   $542 = $541 & $540;
   $543 = ($542|0)==(0);
   if ($543) {
    HEAP32[((11680 + 8|0))>>2] = $540;
    HEAP32[((11680 + 4|0))>>2] = $540;
    HEAP32[((11680 + 12|0))>>2] = -1;
    HEAP32[((11680 + 16|0))>>2] = -1;
    HEAP32[((11680 + 20|0))>>2] = 0;
    HEAP32[((11208 + 444|0))>>2] = 0;
    $544 = (_time((0|0))|0);
    $545 = $544 & -16;
    $546 = $545 ^ 1431655768;
    HEAP32[11680>>2] = $546;
    break;
   } else {
    _abort();
    // unreachable;
   }
  }
 } while(0);
 $547 = (($nb$0) + 48)|0;
 $548 = HEAP32[((11680 + 8|0))>>2]|0;
 $549 = (($nb$0) + 47)|0;
 $550 = (($548) + ($549))|0;
 $551 = (0 - ($548))|0;
 $552 = $550 & $551;
 $553 = ($552>>>0)>($nb$0>>>0);
 if (!($553)) {
  $mem$0 = 0;
  STACKTOP = sp;return ($mem$0|0);
 }
 $554 = HEAP32[((11208 + 440|0))>>2]|0;
 $555 = ($554|0)==(0);
 if (!($555)) {
  $556 = HEAP32[((11208 + 432|0))>>2]|0;
  $557 = (($556) + ($552))|0;
  $558 = ($557>>>0)<=($556>>>0);
  $559 = ($557>>>0)>($554>>>0);
  $or$cond1$i = $558 | $559;
  if ($or$cond1$i) {
   $mem$0 = 0;
   STACKTOP = sp;return ($mem$0|0);
  }
 }
 $560 = HEAP32[((11208 + 444|0))>>2]|0;
 $561 = $560 & 4;
 $562 = ($561|0)==(0);
 L269: do {
  if ($562) {
   $563 = HEAP32[((11208 + 24|0))>>2]|0;
   $564 = ($563|0)==(0|0);
   L271: do {
    if ($564) {
     label = 182;
    } else {
     $sp$0$i$i = ((11208 + 448|0));
     while(1) {
      $565 = HEAP32[$sp$0$i$i>>2]|0;
      $566 = ($565>>>0)>($563>>>0);
      if (!($566)) {
       $567 = (($sp$0$i$i) + 4|0);
       $568 = HEAP32[$567>>2]|0;
       $569 = (($565) + ($568)|0);
       $570 = ($569>>>0)>($563>>>0);
       if ($570) {
        break;
       }
      }
      $571 = (($sp$0$i$i) + 8|0);
      $572 = HEAP32[$571>>2]|0;
      $573 = ($572|0)==(0|0);
      if ($573) {
       label = 182;
       break L271;
      } else {
       $sp$0$i$i = $572;
      }
     }
     $574 = ($sp$0$i$i|0)==(0|0);
     if ($574) {
      label = 182;
     } else {
      $597 = HEAP32[((11208 + 12|0))>>2]|0;
      $598 = (($550) - ($597))|0;
      $599 = $598 & $551;
      $600 = ($599>>>0)<(2147483647);
      if ($600) {
       $601 = (_sbrk(($599|0))|0);
       $602 = HEAP32[$sp$0$i$i>>2]|0;
       $603 = HEAP32[$567>>2]|0;
       $604 = (($602) + ($603)|0);
       $605 = ($601|0)==($604|0);
       $$3$i = $605 ? $599 : 0;
       $$4$i = $605 ? $601 : (-1);
       $br$0$i = $601;$ssize$1$i = $599;$tbase$0$i = $$4$i;$tsize$0$i = $$3$i;
       label = 191;
      } else {
       $tsize$0323841$i = 0;
      }
     }
    }
   } while(0);
   do {
    if ((label|0) == 182) {
     $575 = (_sbrk(0)|0);
     $576 = ($575|0)==((-1)|0);
     if ($576) {
      $tsize$0323841$i = 0;
     } else {
      $577 = $575;
      $578 = HEAP32[((11680 + 4|0))>>2]|0;
      $579 = (($578) + -1)|0;
      $580 = $579 & $577;
      $581 = ($580|0)==(0);
      if ($581) {
       $ssize$0$i = $552;
      } else {
       $582 = (($579) + ($577))|0;
       $583 = (0 - ($578))|0;
       $584 = $582 & $583;
       $585 = (($552) - ($577))|0;
       $586 = (($585) + ($584))|0;
       $ssize$0$i = $586;
      }
      $587 = HEAP32[((11208 + 432|0))>>2]|0;
      $588 = (($587) + ($ssize$0$i))|0;
      $589 = ($ssize$0$i>>>0)>($nb$0>>>0);
      $590 = ($ssize$0$i>>>0)<(2147483647);
      $or$cond$i29 = $589 & $590;
      if ($or$cond$i29) {
       $591 = HEAP32[((11208 + 440|0))>>2]|0;
       $592 = ($591|0)==(0);
       if (!($592)) {
        $593 = ($588>>>0)<=($587>>>0);
        $594 = ($588>>>0)>($591>>>0);
        $or$cond2$i = $593 | $594;
        if ($or$cond2$i) {
         $tsize$0323841$i = 0;
         break;
        }
       }
       $595 = (_sbrk(($ssize$0$i|0))|0);
       $596 = ($595|0)==($575|0);
       $ssize$0$$i = $596 ? $ssize$0$i : 0;
       $$$i = $596 ? $575 : (-1);
       $br$0$i = $595;$ssize$1$i = $ssize$0$i;$tbase$0$i = $$$i;$tsize$0$i = $ssize$0$$i;
       label = 191;
      } else {
       $tsize$0323841$i = 0;
      }
     }
    }
   } while(0);
   L291: do {
    if ((label|0) == 191) {
     $606 = (0 - ($ssize$1$i))|0;
     $607 = ($tbase$0$i|0)==((-1)|0);
     if (!($607)) {
      $tbase$247$i = $tbase$0$i;$tsize$246$i = $tsize$0$i;
      label = 202;
      break L269;
     }
     $608 = ($br$0$i|0)!=((-1)|0);
     $609 = ($ssize$1$i>>>0)<(2147483647);
     $or$cond5$i = $608 & $609;
     $610 = ($ssize$1$i>>>0)<($547>>>0);
     $or$cond6$i = $or$cond5$i & $610;
     do {
      if ($or$cond6$i) {
       $611 = HEAP32[((11680 + 8|0))>>2]|0;
       $612 = (($549) - ($ssize$1$i))|0;
       $613 = (($612) + ($611))|0;
       $614 = (0 - ($611))|0;
       $615 = $613 & $614;
       $616 = ($615>>>0)<(2147483647);
       if ($616) {
        $617 = (_sbrk(($615|0))|0);
        $618 = ($617|0)==((-1)|0);
        if ($618) {
         (_sbrk(($606|0))|0);
         $tsize$0323841$i = $tsize$0$i;
         break L291;
        } else {
         $619 = (($615) + ($ssize$1$i))|0;
         $ssize$2$i = $619;
         break;
        }
       } else {
        $ssize$2$i = $ssize$1$i;
       }
      } else {
       $ssize$2$i = $ssize$1$i;
      }
     } while(0);
     $620 = ($br$0$i|0)==((-1)|0);
     if ($620) {
      $tsize$0323841$i = $tsize$0$i;
     } else {
      $tbase$247$i = $br$0$i;$tsize$246$i = $ssize$2$i;
      label = 202;
      break L269;
     }
    }
   } while(0);
   $621 = HEAP32[((11208 + 444|0))>>2]|0;
   $622 = $621 | 4;
   HEAP32[((11208 + 444|0))>>2] = $622;
   $tsize$1$i = $tsize$0323841$i;
   label = 199;
  } else {
   $tsize$1$i = 0;
   label = 199;
  }
 } while(0);
 if ((label|0) == 199) {
  $623 = ($552>>>0)<(2147483647);
  if ($623) {
   $624 = (_sbrk(($552|0))|0);
   $625 = (_sbrk(0)|0);
   $notlhs$i = ($624|0)!=((-1)|0);
   $notrhs$i = ($625|0)!=((-1)|0);
   $or$cond8$not$i = $notrhs$i & $notlhs$i;
   $626 = ($624>>>0)<($625>>>0);
   $or$cond9$i = $or$cond8$not$i & $626;
   if ($or$cond9$i) {
    $627 = $625;
    $628 = $624;
    $629 = (($627) - ($628))|0;
    $630 = (($nb$0) + 40)|0;
    $631 = ($629>>>0)>($630>>>0);
    $$tsize$1$i = $631 ? $629 : $tsize$1$i;
    if ($631) {
     $tbase$247$i = $624;$tsize$246$i = $$tsize$1$i;
     label = 202;
    }
   }
  }
 }
 if ((label|0) == 202) {
  $632 = HEAP32[((11208 + 432|0))>>2]|0;
  $633 = (($632) + ($tsize$246$i))|0;
  HEAP32[((11208 + 432|0))>>2] = $633;
  $634 = HEAP32[((11208 + 436|0))>>2]|0;
  $635 = ($633>>>0)>($634>>>0);
  if ($635) {
   HEAP32[((11208 + 436|0))>>2] = $633;
  }
  $636 = HEAP32[((11208 + 24|0))>>2]|0;
  $637 = ($636|0)==(0|0);
  L311: do {
   if ($637) {
    $638 = HEAP32[((11208 + 16|0))>>2]|0;
    $639 = ($638|0)==(0|0);
    $640 = ($tbase$247$i>>>0)<($638>>>0);
    $or$cond10$i = $639 | $640;
    if ($or$cond10$i) {
     HEAP32[((11208 + 16|0))>>2] = $tbase$247$i;
    }
    HEAP32[((11208 + 448|0))>>2] = $tbase$247$i;
    HEAP32[((11208 + 452|0))>>2] = $tsize$246$i;
    HEAP32[((11208 + 460|0))>>2] = 0;
    $641 = HEAP32[11680>>2]|0;
    HEAP32[((11208 + 36|0))>>2] = $641;
    HEAP32[((11208 + 32|0))>>2] = -1;
    $i$02$i$i = 0;
    while(1) {
     $642 = $i$02$i$i << 1;
     $643 = ((11208 + ($642<<2)|0) + 40|0);
     $$sum$i$i = (($642) + 3)|0;
     $644 = ((11208 + ($$sum$i$i<<2)|0) + 40|0);
     HEAP32[$644>>2] = $643;
     $$sum1$i$i = (($642) + 2)|0;
     $645 = ((11208 + ($$sum1$i$i<<2)|0) + 40|0);
     HEAP32[$645>>2] = $643;
     $646 = (($i$02$i$i) + 1)|0;
     $exitcond$i$i = ($646|0)==(32);
     if ($exitcond$i$i) {
      break;
     } else {
      $i$02$i$i = $646;
     }
    }
    $647 = (($tsize$246$i) + -40)|0;
    $648 = (($tbase$247$i) + 8|0);
    $649 = $648;
    $650 = $649 & 7;
    $651 = ($650|0)==(0);
    if ($651) {
     $655 = 0;
    } else {
     $652 = (0 - ($649))|0;
     $653 = $652 & 7;
     $655 = $653;
    }
    $654 = (($tbase$247$i) + ($655)|0);
    $656 = (($647) - ($655))|0;
    HEAP32[((11208 + 24|0))>>2] = $654;
    HEAP32[((11208 + 12|0))>>2] = $656;
    $657 = $656 | 1;
    $$sum$i14$i = (($655) + 4)|0;
    $658 = (($tbase$247$i) + ($$sum$i14$i)|0);
    HEAP32[$658>>2] = $657;
    $$sum2$i$i = (($tsize$246$i) + -36)|0;
    $659 = (($tbase$247$i) + ($$sum2$i$i)|0);
    HEAP32[$659>>2] = 40;
    $660 = HEAP32[((11680 + 16|0))>>2]|0;
    HEAP32[((11208 + 28|0))>>2] = $660;
   } else {
    $sp$075$i = ((11208 + 448|0));
    while(1) {
     $661 = HEAP32[$sp$075$i>>2]|0;
     $662 = (($sp$075$i) + 4|0);
     $663 = HEAP32[$662>>2]|0;
     $664 = (($661) + ($663)|0);
     $665 = ($tbase$247$i|0)==($664|0);
     if ($665) {
      label = 214;
      break;
     }
     $666 = (($sp$075$i) + 8|0);
     $667 = HEAP32[$666>>2]|0;
     $668 = ($667|0)==(0|0);
     if ($668) {
      break;
     } else {
      $sp$075$i = $667;
     }
    }
    if ((label|0) == 214) {
     $669 = (($sp$075$i) + 12|0);
     $670 = HEAP32[$669>>2]|0;
     $671 = $670 & 8;
     $672 = ($671|0)==(0);
     if ($672) {
      $673 = ($636>>>0)>=($661>>>0);
      $674 = ($636>>>0)<($tbase$247$i>>>0);
      $or$cond49$i = $673 & $674;
      if ($or$cond49$i) {
       $675 = (($663) + ($tsize$246$i))|0;
       HEAP32[$662>>2] = $675;
       $676 = HEAP32[((11208 + 12|0))>>2]|0;
       $677 = (($676) + ($tsize$246$i))|0;
       $678 = (($636) + 8|0);
       $679 = $678;
       $680 = $679 & 7;
       $681 = ($680|0)==(0);
       if ($681) {
        $685 = 0;
       } else {
        $682 = (0 - ($679))|0;
        $683 = $682 & 7;
        $685 = $683;
       }
       $684 = (($636) + ($685)|0);
       $686 = (($677) - ($685))|0;
       HEAP32[((11208 + 24|0))>>2] = $684;
       HEAP32[((11208 + 12|0))>>2] = $686;
       $687 = $686 | 1;
       $$sum$i18$i = (($685) + 4)|0;
       $688 = (($636) + ($$sum$i18$i)|0);
       HEAP32[$688>>2] = $687;
       $$sum2$i19$i = (($677) + 4)|0;
       $689 = (($636) + ($$sum2$i19$i)|0);
       HEAP32[$689>>2] = 40;
       $690 = HEAP32[((11680 + 16|0))>>2]|0;
       HEAP32[((11208 + 28|0))>>2] = $690;
       break;
      }
     }
    }
    $691 = HEAP32[((11208 + 16|0))>>2]|0;
    $692 = ($tbase$247$i>>>0)<($691>>>0);
    if ($692) {
     HEAP32[((11208 + 16|0))>>2] = $tbase$247$i;
    }
    $693 = (($tbase$247$i) + ($tsize$246$i)|0);
    $sp$168$i = ((11208 + 448|0));
    while(1) {
     $694 = HEAP32[$sp$168$i>>2]|0;
     $695 = ($694|0)==($693|0);
     if ($695) {
      label = 224;
      break;
     }
     $696 = (($sp$168$i) + 8|0);
     $697 = HEAP32[$696>>2]|0;
     $698 = ($697|0)==(0|0);
     if ($698) {
      break;
     } else {
      $sp$168$i = $697;
     }
    }
    if ((label|0) == 224) {
     $699 = (($sp$168$i) + 12|0);
     $700 = HEAP32[$699>>2]|0;
     $701 = $700 & 8;
     $702 = ($701|0)==(0);
     if ($702) {
      HEAP32[$sp$168$i>>2] = $tbase$247$i;
      $703 = (($sp$168$i) + 4|0);
      $704 = HEAP32[$703>>2]|0;
      $705 = (($704) + ($tsize$246$i))|0;
      HEAP32[$703>>2] = $705;
      $706 = (($tbase$247$i) + 8|0);
      $707 = $706;
      $708 = $707 & 7;
      $709 = ($708|0)==(0);
      if ($709) {
       $713 = 0;
      } else {
       $710 = (0 - ($707))|0;
       $711 = $710 & 7;
       $713 = $711;
      }
      $712 = (($tbase$247$i) + ($713)|0);
      $$sum107$i = (($tsize$246$i) + 8)|0;
      $714 = (($tbase$247$i) + ($$sum107$i)|0);
      $715 = $714;
      $716 = $715 & 7;
      $717 = ($716|0)==(0);
      if ($717) {
       $720 = 0;
      } else {
       $718 = (0 - ($715))|0;
       $719 = $718 & 7;
       $720 = $719;
      }
      $$sum108$i = (($720) + ($tsize$246$i))|0;
      $721 = (($tbase$247$i) + ($$sum108$i)|0);
      $722 = $721;
      $723 = $712;
      $724 = (($722) - ($723))|0;
      $$sum$i21$i = (($713) + ($nb$0))|0;
      $725 = (($tbase$247$i) + ($$sum$i21$i)|0);
      $726 = (($724) - ($nb$0))|0;
      $727 = $nb$0 | 3;
      $$sum1$i22$i = (($713) + 4)|0;
      $728 = (($tbase$247$i) + ($$sum1$i22$i)|0);
      HEAP32[$728>>2] = $727;
      $729 = HEAP32[((11208 + 24|0))>>2]|0;
      $730 = ($721|0)==($729|0);
      L338: do {
       if ($730) {
        $731 = HEAP32[((11208 + 12|0))>>2]|0;
        $732 = (($731) + ($726))|0;
        HEAP32[((11208 + 12|0))>>2] = $732;
        HEAP32[((11208 + 24|0))>>2] = $725;
        $733 = $732 | 1;
        $$sum42$i$i = (($$sum$i21$i) + 4)|0;
        $734 = (($tbase$247$i) + ($$sum42$i$i)|0);
        HEAP32[$734>>2] = $733;
       } else {
        $735 = HEAP32[((11208 + 20|0))>>2]|0;
        $736 = ($721|0)==($735|0);
        if ($736) {
         $737 = HEAP32[((11208 + 8|0))>>2]|0;
         $738 = (($737) + ($726))|0;
         HEAP32[((11208 + 8|0))>>2] = $738;
         HEAP32[((11208 + 20|0))>>2] = $725;
         $739 = $738 | 1;
         $$sum40$i$i = (($$sum$i21$i) + 4)|0;
         $740 = (($tbase$247$i) + ($$sum40$i$i)|0);
         HEAP32[$740>>2] = $739;
         $$sum41$i$i = (($738) + ($$sum$i21$i))|0;
         $741 = (($tbase$247$i) + ($$sum41$i$i)|0);
         HEAP32[$741>>2] = $738;
         break;
        }
        $$sum2$i23$i = (($tsize$246$i) + 4)|0;
        $$sum109$i = (($$sum2$i23$i) + ($720))|0;
        $742 = (($tbase$247$i) + ($$sum109$i)|0);
        $743 = HEAP32[$742>>2]|0;
        $744 = $743 & 3;
        $745 = ($744|0)==(1);
        if ($745) {
         $746 = $743 & -8;
         $747 = $743 >>> 3;
         $748 = ($743>>>0)<(256);
         do {
          if ($748) {
           $$sum3738$i$i = $720 | 8;
           $$sum119$i = (($$sum3738$i$i) + ($tsize$246$i))|0;
           $749 = (($tbase$247$i) + ($$sum119$i)|0);
           $750 = HEAP32[$749>>2]|0;
           $$sum39$i$i = (($tsize$246$i) + 12)|0;
           $$sum120$i = (($$sum39$i$i) + ($720))|0;
           $751 = (($tbase$247$i) + ($$sum120$i)|0);
           $752 = HEAP32[$751>>2]|0;
           $753 = $747 << 1;
           $754 = ((11208 + ($753<<2)|0) + 40|0);
           $755 = ($750|0)==($754|0);
           if (!($755)) {
            $756 = HEAP32[((11208 + 16|0))>>2]|0;
            $757 = ($750>>>0)<($756>>>0);
            if ($757) {
             _abort();
             // unreachable;
            }
            $758 = (($750) + 12|0);
            $759 = HEAP32[$758>>2]|0;
            $760 = ($759|0)==($721|0);
            if (!($760)) {
             _abort();
             // unreachable;
            }
           }
           $761 = ($752|0)==($750|0);
           if ($761) {
            $762 = 1 << $747;
            $763 = $762 ^ -1;
            $764 = HEAP32[11208>>2]|0;
            $765 = $764 & $763;
            HEAP32[11208>>2] = $765;
            break;
           }
           $766 = ($752|0)==($754|0);
           if ($766) {
            $$pre57$i$i = (($752) + 8|0);
            $$pre$phi58$i$iZ2D = $$pre57$i$i;
           } else {
            $767 = HEAP32[((11208 + 16|0))>>2]|0;
            $768 = ($752>>>0)<($767>>>0);
            if ($768) {
             _abort();
             // unreachable;
            }
            $769 = (($752) + 8|0);
            $770 = HEAP32[$769>>2]|0;
            $771 = ($770|0)==($721|0);
            if ($771) {
             $$pre$phi58$i$iZ2D = $769;
            } else {
             _abort();
             // unreachable;
            }
           }
           $772 = (($750) + 12|0);
           HEAP32[$772>>2] = $752;
           HEAP32[$$pre$phi58$i$iZ2D>>2] = $750;
          } else {
           $$sum34$i$i = $720 | 24;
           $$sum110$i = (($$sum34$i$i) + ($tsize$246$i))|0;
           $773 = (($tbase$247$i) + ($$sum110$i)|0);
           $774 = HEAP32[$773>>2]|0;
           $$sum5$i$i = (($tsize$246$i) + 12)|0;
           $$sum111$i = (($$sum5$i$i) + ($720))|0;
           $775 = (($tbase$247$i) + ($$sum111$i)|0);
           $776 = HEAP32[$775>>2]|0;
           $777 = ($776|0)==($721|0);
           do {
            if ($777) {
             $$sum67$i$i = $720 | 16;
             $$sum117$i = (($$sum2$i23$i) + ($$sum67$i$i))|0;
             $788 = (($tbase$247$i) + ($$sum117$i)|0);
             $789 = HEAP32[$788>>2]|0;
             $790 = ($789|0)==(0|0);
             if ($790) {
              $$sum118$i = (($$sum67$i$i) + ($tsize$246$i))|0;
              $791 = (($tbase$247$i) + ($$sum118$i)|0);
              $792 = HEAP32[$791>>2]|0;
              $793 = ($792|0)==(0|0);
              if ($793) {
               $R$1$i$i = 0;
               break;
              } else {
               $R$0$i$i = $792;$RP$0$i$i = $791;
              }
             } else {
              $R$0$i$i = $789;$RP$0$i$i = $788;
             }
             while(1) {
              $794 = (($R$0$i$i) + 20|0);
              $795 = HEAP32[$794>>2]|0;
              $796 = ($795|0)==(0|0);
              if (!($796)) {
               $R$0$i$i = $795;$RP$0$i$i = $794;
               continue;
              }
              $797 = (($R$0$i$i) + 16|0);
              $798 = HEAP32[$797>>2]|0;
              $799 = ($798|0)==(0|0);
              if ($799) {
               break;
              } else {
               $R$0$i$i = $798;$RP$0$i$i = $797;
              }
             }
             $800 = HEAP32[((11208 + 16|0))>>2]|0;
             $801 = ($RP$0$i$i>>>0)<($800>>>0);
             if ($801) {
              _abort();
              // unreachable;
             } else {
              HEAP32[$RP$0$i$i>>2] = 0;
              $R$1$i$i = $R$0$i$i;
              break;
             }
            } else {
             $$sum3536$i$i = $720 | 8;
             $$sum112$i = (($$sum3536$i$i) + ($tsize$246$i))|0;
             $778 = (($tbase$247$i) + ($$sum112$i)|0);
             $779 = HEAP32[$778>>2]|0;
             $780 = HEAP32[((11208 + 16|0))>>2]|0;
             $781 = ($779>>>0)<($780>>>0);
             if ($781) {
              _abort();
              // unreachable;
             }
             $782 = (($779) + 12|0);
             $783 = HEAP32[$782>>2]|0;
             $784 = ($783|0)==($721|0);
             if (!($784)) {
              _abort();
              // unreachable;
             }
             $785 = (($776) + 8|0);
             $786 = HEAP32[$785>>2]|0;
             $787 = ($786|0)==($721|0);
             if ($787) {
              HEAP32[$782>>2] = $776;
              HEAP32[$785>>2] = $779;
              $R$1$i$i = $776;
              break;
             } else {
              _abort();
              // unreachable;
             }
            }
           } while(0);
           $802 = ($774|0)==(0|0);
           if (!($802)) {
            $$sum30$i$i = (($tsize$246$i) + 28)|0;
            $$sum113$i = (($$sum30$i$i) + ($720))|0;
            $803 = (($tbase$247$i) + ($$sum113$i)|0);
            $804 = HEAP32[$803>>2]|0;
            $805 = ((11208 + ($804<<2)|0) + 304|0);
            $806 = HEAP32[$805>>2]|0;
            $807 = ($721|0)==($806|0);
            if ($807) {
             HEAP32[$805>>2] = $R$1$i$i;
             $cond$i$i = ($R$1$i$i|0)==(0|0);
             if ($cond$i$i) {
              $808 = 1 << $804;
              $809 = $808 ^ -1;
              $810 = HEAP32[((11208 + 4|0))>>2]|0;
              $811 = $810 & $809;
              HEAP32[((11208 + 4|0))>>2] = $811;
              break;
             }
            } else {
             $812 = HEAP32[((11208 + 16|0))>>2]|0;
             $813 = ($774>>>0)<($812>>>0);
             if ($813) {
              _abort();
              // unreachable;
             }
             $814 = (($774) + 16|0);
             $815 = HEAP32[$814>>2]|0;
             $816 = ($815|0)==($721|0);
             if ($816) {
              HEAP32[$814>>2] = $R$1$i$i;
             } else {
              $817 = (($774) + 20|0);
              HEAP32[$817>>2] = $R$1$i$i;
             }
             $818 = ($R$1$i$i|0)==(0|0);
             if ($818) {
              break;
             }
            }
            $819 = HEAP32[((11208 + 16|0))>>2]|0;
            $820 = ($R$1$i$i>>>0)<($819>>>0);
            if ($820) {
             _abort();
             // unreachable;
            }
            $821 = (($R$1$i$i) + 24|0);
            HEAP32[$821>>2] = $774;
            $$sum3132$i$i = $720 | 16;
            $$sum114$i = (($$sum3132$i$i) + ($tsize$246$i))|0;
            $822 = (($tbase$247$i) + ($$sum114$i)|0);
            $823 = HEAP32[$822>>2]|0;
            $824 = ($823|0)==(0|0);
            do {
             if (!($824)) {
              $825 = HEAP32[((11208 + 16|0))>>2]|0;
              $826 = ($823>>>0)<($825>>>0);
              if ($826) {
               _abort();
               // unreachable;
              } else {
               $827 = (($R$1$i$i) + 16|0);
               HEAP32[$827>>2] = $823;
               $828 = (($823) + 24|0);
               HEAP32[$828>>2] = $R$1$i$i;
               break;
              }
             }
            } while(0);
            $$sum115$i = (($$sum2$i23$i) + ($$sum3132$i$i))|0;
            $829 = (($tbase$247$i) + ($$sum115$i)|0);
            $830 = HEAP32[$829>>2]|0;
            $831 = ($830|0)==(0|0);
            if (!($831)) {
             $832 = HEAP32[((11208 + 16|0))>>2]|0;
             $833 = ($830>>>0)<($832>>>0);
             if ($833) {
              _abort();
              // unreachable;
             } else {
              $834 = (($R$1$i$i) + 20|0);
              HEAP32[$834>>2] = $830;
              $835 = (($830) + 24|0);
              HEAP32[$835>>2] = $R$1$i$i;
              break;
             }
            }
           }
          }
         } while(0);
         $$sum9$i$i = $746 | $720;
         $$sum116$i = (($$sum9$i$i) + ($tsize$246$i))|0;
         $836 = (($tbase$247$i) + ($$sum116$i)|0);
         $837 = (($746) + ($726))|0;
         $oldfirst$0$i$i = $836;$qsize$0$i$i = $837;
        } else {
         $oldfirst$0$i$i = $721;$qsize$0$i$i = $726;
        }
        $838 = (($oldfirst$0$i$i) + 4|0);
        $839 = HEAP32[$838>>2]|0;
        $840 = $839 & -2;
        HEAP32[$838>>2] = $840;
        $841 = $qsize$0$i$i | 1;
        $$sum10$i$i = (($$sum$i21$i) + 4)|0;
        $842 = (($tbase$247$i) + ($$sum10$i$i)|0);
        HEAP32[$842>>2] = $841;
        $$sum11$i24$i = (($qsize$0$i$i) + ($$sum$i21$i))|0;
        $843 = (($tbase$247$i) + ($$sum11$i24$i)|0);
        HEAP32[$843>>2] = $qsize$0$i$i;
        $844 = $qsize$0$i$i >>> 3;
        $845 = ($qsize$0$i$i>>>0)<(256);
        if ($845) {
         $846 = $844 << 1;
         $847 = ((11208 + ($846<<2)|0) + 40|0);
         $848 = HEAP32[11208>>2]|0;
         $849 = 1 << $844;
         $850 = $848 & $849;
         $851 = ($850|0)==(0);
         if ($851) {
          $852 = $848 | $849;
          HEAP32[11208>>2] = $852;
          $$sum26$pre$i$i = (($846) + 2)|0;
          $$pre$i25$i = ((11208 + ($$sum26$pre$i$i<<2)|0) + 40|0);
          $$pre$phi$i26$iZ2D = $$pre$i25$i;$F4$0$i$i = $847;
         } else {
          $$sum29$i$i = (($846) + 2)|0;
          $853 = ((11208 + ($$sum29$i$i<<2)|0) + 40|0);
          $854 = HEAP32[$853>>2]|0;
          $855 = HEAP32[((11208 + 16|0))>>2]|0;
          $856 = ($854>>>0)<($855>>>0);
          if ($856) {
           _abort();
           // unreachable;
          } else {
           $$pre$phi$i26$iZ2D = $853;$F4$0$i$i = $854;
          }
         }
         HEAP32[$$pre$phi$i26$iZ2D>>2] = $725;
         $857 = (($F4$0$i$i) + 12|0);
         HEAP32[$857>>2] = $725;
         $$sum27$i$i = (($$sum$i21$i) + 8)|0;
         $858 = (($tbase$247$i) + ($$sum27$i$i)|0);
         HEAP32[$858>>2] = $F4$0$i$i;
         $$sum28$i$i = (($$sum$i21$i) + 12)|0;
         $859 = (($tbase$247$i) + ($$sum28$i$i)|0);
         HEAP32[$859>>2] = $847;
         break;
        }
        $860 = $qsize$0$i$i >>> 8;
        $861 = ($860|0)==(0);
        if ($861) {
         $I7$0$i$i = 0;
        } else {
         $862 = ($qsize$0$i$i>>>0)>(16777215);
         if ($862) {
          $I7$0$i$i = 31;
         } else {
          $863 = (($860) + 1048320)|0;
          $864 = $863 >>> 16;
          $865 = $864 & 8;
          $866 = $860 << $865;
          $867 = (($866) + 520192)|0;
          $868 = $867 >>> 16;
          $869 = $868 & 4;
          $870 = $869 | $865;
          $871 = $866 << $869;
          $872 = (($871) + 245760)|0;
          $873 = $872 >>> 16;
          $874 = $873 & 2;
          $875 = $870 | $874;
          $876 = (14 - ($875))|0;
          $877 = $871 << $874;
          $878 = $877 >>> 15;
          $879 = (($876) + ($878))|0;
          $880 = $879 << 1;
          $881 = (($879) + 7)|0;
          $882 = $qsize$0$i$i >>> $881;
          $883 = $882 & 1;
          $884 = $883 | $880;
          $I7$0$i$i = $884;
         }
        }
        $885 = ((11208 + ($I7$0$i$i<<2)|0) + 304|0);
        $$sum12$i$i = (($$sum$i21$i) + 28)|0;
        $886 = (($tbase$247$i) + ($$sum12$i$i)|0);
        HEAP32[$886>>2] = $I7$0$i$i;
        $$sum13$i$i = (($$sum$i21$i) + 16)|0;
        $887 = (($tbase$247$i) + ($$sum13$i$i)|0);
        $$sum14$i$i = (($$sum$i21$i) + 20)|0;
        $888 = (($tbase$247$i) + ($$sum14$i$i)|0);
        HEAP32[$888>>2] = 0;
        HEAP32[$887>>2] = 0;
        $889 = HEAP32[((11208 + 4|0))>>2]|0;
        $890 = 1 << $I7$0$i$i;
        $891 = $889 & $890;
        $892 = ($891|0)==(0);
        if ($892) {
         $893 = $889 | $890;
         HEAP32[((11208 + 4|0))>>2] = $893;
         HEAP32[$885>>2] = $725;
         $$sum15$i$i = (($$sum$i21$i) + 24)|0;
         $894 = (($tbase$247$i) + ($$sum15$i$i)|0);
         HEAP32[$894>>2] = $885;
         $$sum16$i$i = (($$sum$i21$i) + 12)|0;
         $895 = (($tbase$247$i) + ($$sum16$i$i)|0);
         HEAP32[$895>>2] = $725;
         $$sum17$i$i = (($$sum$i21$i) + 8)|0;
         $896 = (($tbase$247$i) + ($$sum17$i$i)|0);
         HEAP32[$896>>2] = $725;
         break;
        }
        $897 = HEAP32[$885>>2]|0;
        $898 = ($I7$0$i$i|0)==(31);
        if ($898) {
         $906 = 0;
        } else {
         $899 = $I7$0$i$i >>> 1;
         $900 = (25 - ($899))|0;
         $906 = $900;
        }
        $901 = (($897) + 4|0);
        $902 = HEAP32[$901>>2]|0;
        $903 = $902 & -8;
        $904 = ($903|0)==($qsize$0$i$i|0);
        L435: do {
         if ($904) {
          $T$0$lcssa$i28$i = $897;
         } else {
          $905 = $qsize$0$i$i << $906;
          $K8$052$i$i = $905;$T$051$i$i = $897;
          while(1) {
           $913 = $K8$052$i$i >>> 31;
           $914 = ((($T$051$i$i) + ($913<<2)|0) + 16|0);
           $909 = HEAP32[$914>>2]|0;
           $915 = ($909|0)==(0|0);
           if ($915) {
            break;
           }
           $907 = $K8$052$i$i << 1;
           $908 = (($909) + 4|0);
           $910 = HEAP32[$908>>2]|0;
           $911 = $910 & -8;
           $912 = ($911|0)==($qsize$0$i$i|0);
           if ($912) {
            $T$0$lcssa$i28$i = $909;
            break L435;
           } else {
            $K8$052$i$i = $907;$T$051$i$i = $909;
           }
          }
          $916 = HEAP32[((11208 + 16|0))>>2]|0;
          $917 = ($914>>>0)<($916>>>0);
          if ($917) {
           _abort();
           // unreachable;
          } else {
           HEAP32[$914>>2] = $725;
           $$sum23$i$i = (($$sum$i21$i) + 24)|0;
           $918 = (($tbase$247$i) + ($$sum23$i$i)|0);
           HEAP32[$918>>2] = $T$051$i$i;
           $$sum24$i$i = (($$sum$i21$i) + 12)|0;
           $919 = (($tbase$247$i) + ($$sum24$i$i)|0);
           HEAP32[$919>>2] = $725;
           $$sum25$i$i = (($$sum$i21$i) + 8)|0;
           $920 = (($tbase$247$i) + ($$sum25$i$i)|0);
           HEAP32[$920>>2] = $725;
           break L338;
          }
         }
        } while(0);
        $921 = (($T$0$lcssa$i28$i) + 8|0);
        $922 = HEAP32[$921>>2]|0;
        $923 = HEAP32[((11208 + 16|0))>>2]|0;
        $924 = ($T$0$lcssa$i28$i>>>0)<($923>>>0);
        if ($924) {
         _abort();
         // unreachable;
        }
        $925 = ($922>>>0)<($923>>>0);
        if ($925) {
         _abort();
         // unreachable;
        } else {
         $926 = (($922) + 12|0);
         HEAP32[$926>>2] = $725;
         HEAP32[$921>>2] = $725;
         $$sum20$i$i = (($$sum$i21$i) + 8)|0;
         $927 = (($tbase$247$i) + ($$sum20$i$i)|0);
         HEAP32[$927>>2] = $922;
         $$sum21$i$i = (($$sum$i21$i) + 12)|0;
         $928 = (($tbase$247$i) + ($$sum21$i$i)|0);
         HEAP32[$928>>2] = $T$0$lcssa$i28$i;
         $$sum22$i$i = (($$sum$i21$i) + 24)|0;
         $929 = (($tbase$247$i) + ($$sum22$i$i)|0);
         HEAP32[$929>>2] = 0;
         break;
        }
       }
      } while(0);
      $$sum1819$i$i = $713 | 8;
      $930 = (($tbase$247$i) + ($$sum1819$i$i)|0);
      $mem$0 = $930;
      STACKTOP = sp;return ($mem$0|0);
     }
    }
    $sp$0$i$i$i = ((11208 + 448|0));
    while(1) {
     $931 = HEAP32[$sp$0$i$i$i>>2]|0;
     $932 = ($931>>>0)>($636>>>0);
     if (!($932)) {
      $933 = (($sp$0$i$i$i) + 4|0);
      $934 = HEAP32[$933>>2]|0;
      $935 = (($931) + ($934)|0);
      $936 = ($935>>>0)>($636>>>0);
      if ($936) {
       break;
      }
     }
     $937 = (($sp$0$i$i$i) + 8|0);
     $938 = HEAP32[$937>>2]|0;
     $sp$0$i$i$i = $938;
    }
    $$sum$i15$i = (($934) + -47)|0;
    $$sum1$i16$i = (($934) + -39)|0;
    $939 = (($931) + ($$sum1$i16$i)|0);
    $940 = $939;
    $941 = $940 & 7;
    $942 = ($941|0)==(0);
    if ($942) {
     $945 = 0;
    } else {
     $943 = (0 - ($940))|0;
     $944 = $943 & 7;
     $945 = $944;
    }
    $$sum2$i17$i = (($$sum$i15$i) + ($945))|0;
    $946 = (($931) + ($$sum2$i17$i)|0);
    $947 = (($636) + 16|0);
    $948 = ($946>>>0)<($947>>>0);
    $949 = $948 ? $636 : $946;
    $950 = (($949) + 8|0);
    $951 = (($tsize$246$i) + -40)|0;
    $952 = (($tbase$247$i) + 8|0);
    $953 = $952;
    $954 = $953 & 7;
    $955 = ($954|0)==(0);
    if ($955) {
     $959 = 0;
    } else {
     $956 = (0 - ($953))|0;
     $957 = $956 & 7;
     $959 = $957;
    }
    $958 = (($tbase$247$i) + ($959)|0);
    $960 = (($951) - ($959))|0;
    HEAP32[((11208 + 24|0))>>2] = $958;
    HEAP32[((11208 + 12|0))>>2] = $960;
    $961 = $960 | 1;
    $$sum$i$i$i = (($959) + 4)|0;
    $962 = (($tbase$247$i) + ($$sum$i$i$i)|0);
    HEAP32[$962>>2] = $961;
    $$sum2$i$i$i = (($tsize$246$i) + -36)|0;
    $963 = (($tbase$247$i) + ($$sum2$i$i$i)|0);
    HEAP32[$963>>2] = 40;
    $964 = HEAP32[((11680 + 16|0))>>2]|0;
    HEAP32[((11208 + 28|0))>>2] = $964;
    $965 = (($949) + 4|0);
    HEAP32[$965>>2] = 27;
    ;HEAP32[$950+0>>2]=HEAP32[((11208 + 448|0))+0>>2]|0;HEAP32[$950+4>>2]=HEAP32[((11208 + 448|0))+4>>2]|0;HEAP32[$950+8>>2]=HEAP32[((11208 + 448|0))+8>>2]|0;HEAP32[$950+12>>2]=HEAP32[((11208 + 448|0))+12>>2]|0;
    HEAP32[((11208 + 448|0))>>2] = $tbase$247$i;
    HEAP32[((11208 + 452|0))>>2] = $tsize$246$i;
    HEAP32[((11208 + 460|0))>>2] = 0;
    HEAP32[((11208 + 456|0))>>2] = $950;
    $966 = (($949) + 28|0);
    HEAP32[$966>>2] = 7;
    $967 = (($949) + 32|0);
    $968 = ($967>>>0)<($935>>>0);
    if ($968) {
     $970 = $966;
     while(1) {
      $969 = (($970) + 4|0);
      HEAP32[$969>>2] = 7;
      $971 = (($970) + 8|0);
      $972 = ($971>>>0)<($935>>>0);
      if ($972) {
       $970 = $969;
      } else {
       break;
      }
     }
    }
    $973 = ($949|0)==($636|0);
    if (!($973)) {
     $974 = $949;
     $975 = $636;
     $976 = (($974) - ($975))|0;
     $977 = (($636) + ($976)|0);
     $$sum3$i$i = (($976) + 4)|0;
     $978 = (($636) + ($$sum3$i$i)|0);
     $979 = HEAP32[$978>>2]|0;
     $980 = $979 & -2;
     HEAP32[$978>>2] = $980;
     $981 = $976 | 1;
     $982 = (($636) + 4|0);
     HEAP32[$982>>2] = $981;
     HEAP32[$977>>2] = $976;
     $983 = $976 >>> 3;
     $984 = ($976>>>0)<(256);
     if ($984) {
      $985 = $983 << 1;
      $986 = ((11208 + ($985<<2)|0) + 40|0);
      $987 = HEAP32[11208>>2]|0;
      $988 = 1 << $983;
      $989 = $987 & $988;
      $990 = ($989|0)==(0);
      if ($990) {
       $991 = $987 | $988;
       HEAP32[11208>>2] = $991;
       $$sum10$pre$i$i = (($985) + 2)|0;
       $$pre$i$i = ((11208 + ($$sum10$pre$i$i<<2)|0) + 40|0);
       $$pre$phi$i$iZ2D = $$pre$i$i;$F$0$i$i = $986;
      } else {
       $$sum11$i$i = (($985) + 2)|0;
       $992 = ((11208 + ($$sum11$i$i<<2)|0) + 40|0);
       $993 = HEAP32[$992>>2]|0;
       $994 = HEAP32[((11208 + 16|0))>>2]|0;
       $995 = ($993>>>0)<($994>>>0);
       if ($995) {
        _abort();
        // unreachable;
       } else {
        $$pre$phi$i$iZ2D = $992;$F$0$i$i = $993;
       }
      }
      HEAP32[$$pre$phi$i$iZ2D>>2] = $636;
      $996 = (($F$0$i$i) + 12|0);
      HEAP32[$996>>2] = $636;
      $997 = (($636) + 8|0);
      HEAP32[$997>>2] = $F$0$i$i;
      $998 = (($636) + 12|0);
      HEAP32[$998>>2] = $986;
      break;
     }
     $999 = $976 >>> 8;
     $1000 = ($999|0)==(0);
     if ($1000) {
      $I1$0$i$i = 0;
     } else {
      $1001 = ($976>>>0)>(16777215);
      if ($1001) {
       $I1$0$i$i = 31;
      } else {
       $1002 = (($999) + 1048320)|0;
       $1003 = $1002 >>> 16;
       $1004 = $1003 & 8;
       $1005 = $999 << $1004;
       $1006 = (($1005) + 520192)|0;
       $1007 = $1006 >>> 16;
       $1008 = $1007 & 4;
       $1009 = $1008 | $1004;
       $1010 = $1005 << $1008;
       $1011 = (($1010) + 245760)|0;
       $1012 = $1011 >>> 16;
       $1013 = $1012 & 2;
       $1014 = $1009 | $1013;
       $1015 = (14 - ($1014))|0;
       $1016 = $1010 << $1013;
       $1017 = $1016 >>> 15;
       $1018 = (($1015) + ($1017))|0;
       $1019 = $1018 << 1;
       $1020 = (($1018) + 7)|0;
       $1021 = $976 >>> $1020;
       $1022 = $1021 & 1;
       $1023 = $1022 | $1019;
       $I1$0$i$i = $1023;
      }
     }
     $1024 = ((11208 + ($I1$0$i$i<<2)|0) + 304|0);
     $1025 = (($636) + 28|0);
     $I1$0$c$i$i = $I1$0$i$i;
     HEAP32[$1025>>2] = $I1$0$c$i$i;
     $1026 = (($636) + 20|0);
     HEAP32[$1026>>2] = 0;
     $1027 = (($636) + 16|0);
     HEAP32[$1027>>2] = 0;
     $1028 = HEAP32[((11208 + 4|0))>>2]|0;
     $1029 = 1 << $I1$0$i$i;
     $1030 = $1028 & $1029;
     $1031 = ($1030|0)==(0);
     if ($1031) {
      $1032 = $1028 | $1029;
      HEAP32[((11208 + 4|0))>>2] = $1032;
      HEAP32[$1024>>2] = $636;
      $1033 = (($636) + 24|0);
      HEAP32[$1033>>2] = $1024;
      $1034 = (($636) + 12|0);
      HEAP32[$1034>>2] = $636;
      $1035 = (($636) + 8|0);
      HEAP32[$1035>>2] = $636;
      break;
     }
     $1036 = HEAP32[$1024>>2]|0;
     $1037 = ($I1$0$i$i|0)==(31);
     if ($1037) {
      $1045 = 0;
     } else {
      $1038 = $I1$0$i$i >>> 1;
      $1039 = (25 - ($1038))|0;
      $1045 = $1039;
     }
     $1040 = (($1036) + 4|0);
     $1041 = HEAP32[$1040>>2]|0;
     $1042 = $1041 & -8;
     $1043 = ($1042|0)==($976|0);
     L489: do {
      if ($1043) {
       $T$0$lcssa$i$i = $1036;
      } else {
       $1044 = $976 << $1045;
       $K2$014$i$i = $1044;$T$013$i$i = $1036;
       while(1) {
        $1052 = $K2$014$i$i >>> 31;
        $1053 = ((($T$013$i$i) + ($1052<<2)|0) + 16|0);
        $1048 = HEAP32[$1053>>2]|0;
        $1054 = ($1048|0)==(0|0);
        if ($1054) {
         break;
        }
        $1046 = $K2$014$i$i << 1;
        $1047 = (($1048) + 4|0);
        $1049 = HEAP32[$1047>>2]|0;
        $1050 = $1049 & -8;
        $1051 = ($1050|0)==($976|0);
        if ($1051) {
         $T$0$lcssa$i$i = $1048;
         break L489;
        } else {
         $K2$014$i$i = $1046;$T$013$i$i = $1048;
        }
       }
       $1055 = HEAP32[((11208 + 16|0))>>2]|0;
       $1056 = ($1053>>>0)<($1055>>>0);
       if ($1056) {
        _abort();
        // unreachable;
       } else {
        HEAP32[$1053>>2] = $636;
        $1057 = (($636) + 24|0);
        HEAP32[$1057>>2] = $T$013$i$i;
        $1058 = (($636) + 12|0);
        HEAP32[$1058>>2] = $636;
        $1059 = (($636) + 8|0);
        HEAP32[$1059>>2] = $636;
        break L311;
       }
      }
     } while(0);
     $1060 = (($T$0$lcssa$i$i) + 8|0);
     $1061 = HEAP32[$1060>>2]|0;
     $1062 = HEAP32[((11208 + 16|0))>>2]|0;
     $1063 = ($T$0$lcssa$i$i>>>0)<($1062>>>0);
     if ($1063) {
      _abort();
      // unreachable;
     }
     $1064 = ($1061>>>0)<($1062>>>0);
     if ($1064) {
      _abort();
      // unreachable;
     } else {
      $1065 = (($1061) + 12|0);
      HEAP32[$1065>>2] = $636;
      HEAP32[$1060>>2] = $636;
      $1066 = (($636) + 8|0);
      HEAP32[$1066>>2] = $1061;
      $1067 = (($636) + 12|0);
      HEAP32[$1067>>2] = $T$0$lcssa$i$i;
      $1068 = (($636) + 24|0);
      HEAP32[$1068>>2] = 0;
      break;
     }
    }
   }
  } while(0);
  $1069 = HEAP32[((11208 + 12|0))>>2]|0;
  $1070 = ($1069>>>0)>($nb$0>>>0);
  if ($1070) {
   $1071 = (($1069) - ($nb$0))|0;
   HEAP32[((11208 + 12|0))>>2] = $1071;
   $1072 = HEAP32[((11208 + 24|0))>>2]|0;
   $1073 = (($1072) + ($nb$0)|0);
   HEAP32[((11208 + 24|0))>>2] = $1073;
   $1074 = $1071 | 1;
   $$sum$i32 = (($nb$0) + 4)|0;
   $1075 = (($1072) + ($$sum$i32)|0);
   HEAP32[$1075>>2] = $1074;
   $1076 = $nb$0 | 3;
   $1077 = (($1072) + 4|0);
   HEAP32[$1077>>2] = $1076;
   $1078 = (($1072) + 8|0);
   $mem$0 = $1078;
   STACKTOP = sp;return ($mem$0|0);
  }
 }
 $1079 = (___errno_location()|0);
 HEAP32[$1079>>2] = 12;
 $mem$0 = 0;
 STACKTOP = sp;return ($mem$0|0);
}
function _free($mem) {
 $mem = $mem|0;
 var $$pre = 0, $$pre$phi68Z2D = 0, $$pre$phi70Z2D = 0, $$pre$phiZ2D = 0, $$pre67 = 0, $$pre69 = 0, $$sum = 0, $$sum16$pre = 0, $$sum17 = 0, $$sum18 = 0, $$sum19 = 0, $$sum2 = 0, $$sum20 = 0, $$sum2324 = 0, $$sum25 = 0, $$sum26 = 0, $$sum28 = 0, $$sum29 = 0, $$sum3 = 0, $$sum30 = 0;
 var $$sum31 = 0, $$sum32 = 0, $$sum33 = 0, $$sum34 = 0, $$sum35 = 0, $$sum36 = 0, $$sum37 = 0, $$sum5 = 0, $$sum67 = 0, $$sum8 = 0, $$sum9 = 0, $0 = 0, $1 = 0, $10 = 0, $100 = 0, $101 = 0, $102 = 0, $103 = 0, $104 = 0, $105 = 0;
 var $106 = 0, $107 = 0, $108 = 0, $109 = 0, $11 = 0, $110 = 0, $111 = 0, $112 = 0, $113 = 0, $114 = 0, $115 = 0, $116 = 0, $117 = 0, $118 = 0, $119 = 0, $12 = 0, $120 = 0, $121 = 0, $122 = 0, $123 = 0;
 var $124 = 0, $125 = 0, $126 = 0, $127 = 0, $128 = 0, $129 = 0, $13 = 0, $130 = 0, $131 = 0, $132 = 0, $133 = 0, $134 = 0, $135 = 0, $136 = 0, $137 = 0, $138 = 0, $139 = 0, $14 = 0, $140 = 0, $141 = 0;
 var $142 = 0, $143 = 0, $144 = 0, $145 = 0, $146 = 0, $147 = 0, $148 = 0, $149 = 0, $15 = 0, $150 = 0, $151 = 0, $152 = 0, $153 = 0, $154 = 0, $155 = 0, $156 = 0, $157 = 0, $158 = 0, $159 = 0, $16 = 0;
 var $160 = 0, $161 = 0, $162 = 0, $163 = 0, $164 = 0, $165 = 0, $166 = 0, $167 = 0, $168 = 0, $169 = 0, $17 = 0, $170 = 0, $171 = 0, $172 = 0, $173 = 0, $174 = 0, $175 = 0, $176 = 0, $177 = 0, $178 = 0;
 var $179 = 0, $18 = 0, $180 = 0, $181 = 0, $182 = 0, $183 = 0, $184 = 0, $185 = 0, $186 = 0, $187 = 0, $188 = 0, $189 = 0, $19 = 0, $190 = 0, $191 = 0, $192 = 0, $193 = 0, $194 = 0, $195 = 0, $196 = 0;
 var $197 = 0, $198 = 0, $199 = 0, $2 = 0, $20 = 0, $200 = 0, $201 = 0, $202 = 0, $203 = 0, $204 = 0, $205 = 0, $206 = 0, $207 = 0, $208 = 0, $209 = 0, $21 = 0, $210 = 0, $211 = 0, $212 = 0, $213 = 0;
 var $214 = 0, $215 = 0, $216 = 0, $217 = 0, $218 = 0, $219 = 0, $22 = 0, $220 = 0, $221 = 0, $222 = 0, $223 = 0, $224 = 0, $225 = 0, $226 = 0, $227 = 0, $228 = 0, $229 = 0, $23 = 0, $230 = 0, $231 = 0;
 var $232 = 0, $233 = 0, $234 = 0, $235 = 0, $236 = 0, $237 = 0, $238 = 0, $239 = 0, $24 = 0, $240 = 0, $241 = 0, $242 = 0, $243 = 0, $244 = 0, $245 = 0, $246 = 0, $247 = 0, $248 = 0, $249 = 0, $25 = 0;
 var $250 = 0, $251 = 0, $252 = 0, $253 = 0, $254 = 0, $255 = 0, $256 = 0, $257 = 0, $258 = 0, $259 = 0, $26 = 0, $260 = 0, $261 = 0, $262 = 0, $263 = 0, $264 = 0, $265 = 0, $266 = 0, $267 = 0, $268 = 0;
 var $269 = 0, $27 = 0, $270 = 0, $271 = 0, $272 = 0, $273 = 0, $274 = 0, $275 = 0, $276 = 0, $277 = 0, $278 = 0, $279 = 0, $28 = 0, $280 = 0, $281 = 0, $282 = 0, $283 = 0, $284 = 0, $285 = 0, $286 = 0;
 var $287 = 0, $288 = 0, $289 = 0, $29 = 0, $290 = 0, $291 = 0, $292 = 0, $293 = 0, $294 = 0, $295 = 0, $296 = 0, $297 = 0, $298 = 0, $299 = 0, $3 = 0, $30 = 0, $300 = 0, $301 = 0, $302 = 0, $303 = 0;
 var $304 = 0, $305 = 0, $306 = 0, $307 = 0, $308 = 0, $309 = 0, $31 = 0, $310 = 0, $311 = 0, $312 = 0, $313 = 0, $314 = 0, $315 = 0, $316 = 0, $317 = 0, $318 = 0, $319 = 0, $32 = 0, $320 = 0, $321 = 0;
 var $322 = 0, $323 = 0, $324 = 0, $33 = 0, $34 = 0, $35 = 0, $36 = 0, $37 = 0, $38 = 0, $39 = 0, $4 = 0, $40 = 0, $41 = 0, $42 = 0, $43 = 0, $44 = 0, $45 = 0, $46 = 0, $47 = 0, $48 = 0;
 var $49 = 0, $5 = 0, $50 = 0, $51 = 0, $52 = 0, $53 = 0, $54 = 0, $55 = 0, $56 = 0, $57 = 0, $58 = 0, $59 = 0, $6 = 0, $60 = 0, $61 = 0, $62 = 0, $63 = 0, $64 = 0, $65 = 0, $66 = 0;
 var $67 = 0, $68 = 0, $69 = 0, $7 = 0, $70 = 0, $71 = 0, $72 = 0, $73 = 0, $74 = 0, $75 = 0, $76 = 0, $77 = 0, $78 = 0, $79 = 0, $8 = 0, $80 = 0, $81 = 0, $82 = 0, $83 = 0, $84 = 0;
 var $85 = 0, $86 = 0, $87 = 0, $88 = 0, $89 = 0, $9 = 0, $90 = 0, $91 = 0, $92 = 0, $93 = 0, $94 = 0, $95 = 0, $96 = 0, $97 = 0, $98 = 0, $99 = 0, $F16$0 = 0, $I18$0 = 0, $I18$0$c = 0, $K19$057 = 0;
 var $R$0 = 0, $R$1 = 0, $R7$0 = 0, $R7$1 = 0, $RP$0 = 0, $RP9$0 = 0, $T$0$lcssa = 0, $T$056 = 0, $cond = 0, $cond54 = 0, $p$0 = 0, $psize$0 = 0, $psize$1 = 0, $sp$0$i = 0, $sp$0$in$i = 0, label = 0, sp = 0;
 sp = STACKTOP;
 $0 = ($mem|0)==(0|0);
 if ($0) {
  STACKTOP = sp;return;
 }
 $1 = (($mem) + -8|0);
 $2 = HEAP32[((11208 + 16|0))>>2]|0;
 $3 = ($1>>>0)<($2>>>0);
 if ($3) {
  _abort();
  // unreachable;
 }
 $4 = (($mem) + -4|0);
 $5 = HEAP32[$4>>2]|0;
 $6 = $5 & 3;
 $7 = ($6|0)==(1);
 if ($7) {
  _abort();
  // unreachable;
 }
 $8 = $5 & -8;
 $$sum = (($8) + -8)|0;
 $9 = (($mem) + ($$sum)|0);
 $10 = $5 & 1;
 $11 = ($10|0)==(0);
 do {
  if ($11) {
   $12 = HEAP32[$1>>2]|0;
   $13 = ($6|0)==(0);
   if ($13) {
    STACKTOP = sp;return;
   }
   $$sum2 = (-8 - ($12))|0;
   $14 = (($mem) + ($$sum2)|0);
   $15 = (($12) + ($8))|0;
   $16 = ($14>>>0)<($2>>>0);
   if ($16) {
    _abort();
    // unreachable;
   }
   $17 = HEAP32[((11208 + 20|0))>>2]|0;
   $18 = ($14|0)==($17|0);
   if ($18) {
    $$sum3 = (($8) + -4)|0;
    $104 = (($mem) + ($$sum3)|0);
    $105 = HEAP32[$104>>2]|0;
    $106 = $105 & 3;
    $107 = ($106|0)==(3);
    if (!($107)) {
     $p$0 = $14;$psize$0 = $15;
     break;
    }
    HEAP32[((11208 + 8|0))>>2] = $15;
    $108 = HEAP32[$104>>2]|0;
    $109 = $108 & -2;
    HEAP32[$104>>2] = $109;
    $110 = $15 | 1;
    $$sum26 = (($$sum2) + 4)|0;
    $111 = (($mem) + ($$sum26)|0);
    HEAP32[$111>>2] = $110;
    HEAP32[$9>>2] = $15;
    STACKTOP = sp;return;
   }
   $19 = $12 >>> 3;
   $20 = ($12>>>0)<(256);
   if ($20) {
    $$sum36 = (($$sum2) + 8)|0;
    $21 = (($mem) + ($$sum36)|0);
    $22 = HEAP32[$21>>2]|0;
    $$sum37 = (($$sum2) + 12)|0;
    $23 = (($mem) + ($$sum37)|0);
    $24 = HEAP32[$23>>2]|0;
    $25 = $19 << 1;
    $26 = ((11208 + ($25<<2)|0) + 40|0);
    $27 = ($22|0)==($26|0);
    if (!($27)) {
     $28 = ($22>>>0)<($2>>>0);
     if ($28) {
      _abort();
      // unreachable;
     }
     $29 = (($22) + 12|0);
     $30 = HEAP32[$29>>2]|0;
     $31 = ($30|0)==($14|0);
     if (!($31)) {
      _abort();
      // unreachable;
     }
    }
    $32 = ($24|0)==($22|0);
    if ($32) {
     $33 = 1 << $19;
     $34 = $33 ^ -1;
     $35 = HEAP32[11208>>2]|0;
     $36 = $35 & $34;
     HEAP32[11208>>2] = $36;
     $p$0 = $14;$psize$0 = $15;
     break;
    }
    $37 = ($24|0)==($26|0);
    if ($37) {
     $$pre69 = (($24) + 8|0);
     $$pre$phi70Z2D = $$pre69;
    } else {
     $38 = ($24>>>0)<($2>>>0);
     if ($38) {
      _abort();
      // unreachable;
     }
     $39 = (($24) + 8|0);
     $40 = HEAP32[$39>>2]|0;
     $41 = ($40|0)==($14|0);
     if ($41) {
      $$pre$phi70Z2D = $39;
     } else {
      _abort();
      // unreachable;
     }
    }
    $42 = (($22) + 12|0);
    HEAP32[$42>>2] = $24;
    HEAP32[$$pre$phi70Z2D>>2] = $22;
    $p$0 = $14;$psize$0 = $15;
    break;
   }
   $$sum28 = (($$sum2) + 24)|0;
   $43 = (($mem) + ($$sum28)|0);
   $44 = HEAP32[$43>>2]|0;
   $$sum29 = (($$sum2) + 12)|0;
   $45 = (($mem) + ($$sum29)|0);
   $46 = HEAP32[$45>>2]|0;
   $47 = ($46|0)==($14|0);
   do {
    if ($47) {
     $$sum31 = (($$sum2) + 20)|0;
     $57 = (($mem) + ($$sum31)|0);
     $58 = HEAP32[$57>>2]|0;
     $59 = ($58|0)==(0|0);
     if ($59) {
      $$sum30 = (($$sum2) + 16)|0;
      $60 = (($mem) + ($$sum30)|0);
      $61 = HEAP32[$60>>2]|0;
      $62 = ($61|0)==(0|0);
      if ($62) {
       $R$1 = 0;
       break;
      } else {
       $R$0 = $61;$RP$0 = $60;
      }
     } else {
      $R$0 = $58;$RP$0 = $57;
     }
     while(1) {
      $63 = (($R$0) + 20|0);
      $64 = HEAP32[$63>>2]|0;
      $65 = ($64|0)==(0|0);
      if (!($65)) {
       $R$0 = $64;$RP$0 = $63;
       continue;
      }
      $66 = (($R$0) + 16|0);
      $67 = HEAP32[$66>>2]|0;
      $68 = ($67|0)==(0|0);
      if ($68) {
       break;
      } else {
       $R$0 = $67;$RP$0 = $66;
      }
     }
     $69 = ($RP$0>>>0)<($2>>>0);
     if ($69) {
      _abort();
      // unreachable;
     } else {
      HEAP32[$RP$0>>2] = 0;
      $R$1 = $R$0;
      break;
     }
    } else {
     $$sum35 = (($$sum2) + 8)|0;
     $48 = (($mem) + ($$sum35)|0);
     $49 = HEAP32[$48>>2]|0;
     $50 = ($49>>>0)<($2>>>0);
     if ($50) {
      _abort();
      // unreachable;
     }
     $51 = (($49) + 12|0);
     $52 = HEAP32[$51>>2]|0;
     $53 = ($52|0)==($14|0);
     if (!($53)) {
      _abort();
      // unreachable;
     }
     $54 = (($46) + 8|0);
     $55 = HEAP32[$54>>2]|0;
     $56 = ($55|0)==($14|0);
     if ($56) {
      HEAP32[$51>>2] = $46;
      HEAP32[$54>>2] = $49;
      $R$1 = $46;
      break;
     } else {
      _abort();
      // unreachable;
     }
    }
   } while(0);
   $70 = ($44|0)==(0|0);
   if ($70) {
    $p$0 = $14;$psize$0 = $15;
   } else {
    $$sum32 = (($$sum2) + 28)|0;
    $71 = (($mem) + ($$sum32)|0);
    $72 = HEAP32[$71>>2]|0;
    $73 = ((11208 + ($72<<2)|0) + 304|0);
    $74 = HEAP32[$73>>2]|0;
    $75 = ($14|0)==($74|0);
    if ($75) {
     HEAP32[$73>>2] = $R$1;
     $cond = ($R$1|0)==(0|0);
     if ($cond) {
      $76 = 1 << $72;
      $77 = $76 ^ -1;
      $78 = HEAP32[((11208 + 4|0))>>2]|0;
      $79 = $78 & $77;
      HEAP32[((11208 + 4|0))>>2] = $79;
      $p$0 = $14;$psize$0 = $15;
      break;
     }
    } else {
     $80 = HEAP32[((11208 + 16|0))>>2]|0;
     $81 = ($44>>>0)<($80>>>0);
     if ($81) {
      _abort();
      // unreachable;
     }
     $82 = (($44) + 16|0);
     $83 = HEAP32[$82>>2]|0;
     $84 = ($83|0)==($14|0);
     if ($84) {
      HEAP32[$82>>2] = $R$1;
     } else {
      $85 = (($44) + 20|0);
      HEAP32[$85>>2] = $R$1;
     }
     $86 = ($R$1|0)==(0|0);
     if ($86) {
      $p$0 = $14;$psize$0 = $15;
      break;
     }
    }
    $87 = HEAP32[((11208 + 16|0))>>2]|0;
    $88 = ($R$1>>>0)<($87>>>0);
    if ($88) {
     _abort();
     // unreachable;
    }
    $89 = (($R$1) + 24|0);
    HEAP32[$89>>2] = $44;
    $$sum33 = (($$sum2) + 16)|0;
    $90 = (($mem) + ($$sum33)|0);
    $91 = HEAP32[$90>>2]|0;
    $92 = ($91|0)==(0|0);
    do {
     if (!($92)) {
      $93 = HEAP32[((11208 + 16|0))>>2]|0;
      $94 = ($91>>>0)<($93>>>0);
      if ($94) {
       _abort();
       // unreachable;
      } else {
       $95 = (($R$1) + 16|0);
       HEAP32[$95>>2] = $91;
       $96 = (($91) + 24|0);
       HEAP32[$96>>2] = $R$1;
       break;
      }
     }
    } while(0);
    $$sum34 = (($$sum2) + 20)|0;
    $97 = (($mem) + ($$sum34)|0);
    $98 = HEAP32[$97>>2]|0;
    $99 = ($98|0)==(0|0);
    if ($99) {
     $p$0 = $14;$psize$0 = $15;
    } else {
     $100 = HEAP32[((11208 + 16|0))>>2]|0;
     $101 = ($98>>>0)<($100>>>0);
     if ($101) {
      _abort();
      // unreachable;
     } else {
      $102 = (($R$1) + 20|0);
      HEAP32[$102>>2] = $98;
      $103 = (($98) + 24|0);
      HEAP32[$103>>2] = $R$1;
      $p$0 = $14;$psize$0 = $15;
      break;
     }
    }
   }
  } else {
   $p$0 = $1;$psize$0 = $8;
  }
 } while(0);
 $112 = ($p$0>>>0)<($9>>>0);
 if (!($112)) {
  _abort();
  // unreachable;
 }
 $$sum25 = (($8) + -4)|0;
 $113 = (($mem) + ($$sum25)|0);
 $114 = HEAP32[$113>>2]|0;
 $115 = $114 & 1;
 $116 = ($115|0)==(0);
 if ($116) {
  _abort();
  // unreachable;
 }
 $117 = $114 & 2;
 $118 = ($117|0)==(0);
 if ($118) {
  $119 = HEAP32[((11208 + 24|0))>>2]|0;
  $120 = ($9|0)==($119|0);
  if ($120) {
   $121 = HEAP32[((11208 + 12|0))>>2]|0;
   $122 = (($121) + ($psize$0))|0;
   HEAP32[((11208 + 12|0))>>2] = $122;
   HEAP32[((11208 + 24|0))>>2] = $p$0;
   $123 = $122 | 1;
   $124 = (($p$0) + 4|0);
   HEAP32[$124>>2] = $123;
   $125 = HEAP32[((11208 + 20|0))>>2]|0;
   $126 = ($p$0|0)==($125|0);
   if (!($126)) {
    STACKTOP = sp;return;
   }
   HEAP32[((11208 + 20|0))>>2] = 0;
   HEAP32[((11208 + 8|0))>>2] = 0;
   STACKTOP = sp;return;
  }
  $127 = HEAP32[((11208 + 20|0))>>2]|0;
  $128 = ($9|0)==($127|0);
  if ($128) {
   $129 = HEAP32[((11208 + 8|0))>>2]|0;
   $130 = (($129) + ($psize$0))|0;
   HEAP32[((11208 + 8|0))>>2] = $130;
   HEAP32[((11208 + 20|0))>>2] = $p$0;
   $131 = $130 | 1;
   $132 = (($p$0) + 4|0);
   HEAP32[$132>>2] = $131;
   $133 = (($p$0) + ($130)|0);
   HEAP32[$133>>2] = $130;
   STACKTOP = sp;return;
  }
  $134 = $114 & -8;
  $135 = (($134) + ($psize$0))|0;
  $136 = $114 >>> 3;
  $137 = ($114>>>0)<(256);
  do {
   if ($137) {
    $138 = (($mem) + ($8)|0);
    $139 = HEAP32[$138>>2]|0;
    $$sum2324 = $8 | 4;
    $140 = (($mem) + ($$sum2324)|0);
    $141 = HEAP32[$140>>2]|0;
    $142 = $136 << 1;
    $143 = ((11208 + ($142<<2)|0) + 40|0);
    $144 = ($139|0)==($143|0);
    if (!($144)) {
     $145 = HEAP32[((11208 + 16|0))>>2]|0;
     $146 = ($139>>>0)<($145>>>0);
     if ($146) {
      _abort();
      // unreachable;
     }
     $147 = (($139) + 12|0);
     $148 = HEAP32[$147>>2]|0;
     $149 = ($148|0)==($9|0);
     if (!($149)) {
      _abort();
      // unreachable;
     }
    }
    $150 = ($141|0)==($139|0);
    if ($150) {
     $151 = 1 << $136;
     $152 = $151 ^ -1;
     $153 = HEAP32[11208>>2]|0;
     $154 = $153 & $152;
     HEAP32[11208>>2] = $154;
     break;
    }
    $155 = ($141|0)==($143|0);
    if ($155) {
     $$pre67 = (($141) + 8|0);
     $$pre$phi68Z2D = $$pre67;
    } else {
     $156 = HEAP32[((11208 + 16|0))>>2]|0;
     $157 = ($141>>>0)<($156>>>0);
     if ($157) {
      _abort();
      // unreachable;
     }
     $158 = (($141) + 8|0);
     $159 = HEAP32[$158>>2]|0;
     $160 = ($159|0)==($9|0);
     if ($160) {
      $$pre$phi68Z2D = $158;
     } else {
      _abort();
      // unreachable;
     }
    }
    $161 = (($139) + 12|0);
    HEAP32[$161>>2] = $141;
    HEAP32[$$pre$phi68Z2D>>2] = $139;
   } else {
    $$sum5 = (($8) + 16)|0;
    $162 = (($mem) + ($$sum5)|0);
    $163 = HEAP32[$162>>2]|0;
    $$sum67 = $8 | 4;
    $164 = (($mem) + ($$sum67)|0);
    $165 = HEAP32[$164>>2]|0;
    $166 = ($165|0)==($9|0);
    do {
     if ($166) {
      $$sum9 = (($8) + 12)|0;
      $177 = (($mem) + ($$sum9)|0);
      $178 = HEAP32[$177>>2]|0;
      $179 = ($178|0)==(0|0);
      if ($179) {
       $$sum8 = (($8) + 8)|0;
       $180 = (($mem) + ($$sum8)|0);
       $181 = HEAP32[$180>>2]|0;
       $182 = ($181|0)==(0|0);
       if ($182) {
        $R7$1 = 0;
        break;
       } else {
        $R7$0 = $181;$RP9$0 = $180;
       }
      } else {
       $R7$0 = $178;$RP9$0 = $177;
      }
      while(1) {
       $183 = (($R7$0) + 20|0);
       $184 = HEAP32[$183>>2]|0;
       $185 = ($184|0)==(0|0);
       if (!($185)) {
        $R7$0 = $184;$RP9$0 = $183;
        continue;
       }
       $186 = (($R7$0) + 16|0);
       $187 = HEAP32[$186>>2]|0;
       $188 = ($187|0)==(0|0);
       if ($188) {
        break;
       } else {
        $R7$0 = $187;$RP9$0 = $186;
       }
      }
      $189 = HEAP32[((11208 + 16|0))>>2]|0;
      $190 = ($RP9$0>>>0)<($189>>>0);
      if ($190) {
       _abort();
       // unreachable;
      } else {
       HEAP32[$RP9$0>>2] = 0;
       $R7$1 = $R7$0;
       break;
      }
     } else {
      $167 = (($mem) + ($8)|0);
      $168 = HEAP32[$167>>2]|0;
      $169 = HEAP32[((11208 + 16|0))>>2]|0;
      $170 = ($168>>>0)<($169>>>0);
      if ($170) {
       _abort();
       // unreachable;
      }
      $171 = (($168) + 12|0);
      $172 = HEAP32[$171>>2]|0;
      $173 = ($172|0)==($9|0);
      if (!($173)) {
       _abort();
       // unreachable;
      }
      $174 = (($165) + 8|0);
      $175 = HEAP32[$174>>2]|0;
      $176 = ($175|0)==($9|0);
      if ($176) {
       HEAP32[$171>>2] = $165;
       HEAP32[$174>>2] = $168;
       $R7$1 = $165;
       break;
      } else {
       _abort();
       // unreachable;
      }
     }
    } while(0);
    $191 = ($163|0)==(0|0);
    if (!($191)) {
     $$sum18 = (($8) + 20)|0;
     $192 = (($mem) + ($$sum18)|0);
     $193 = HEAP32[$192>>2]|0;
     $194 = ((11208 + ($193<<2)|0) + 304|0);
     $195 = HEAP32[$194>>2]|0;
     $196 = ($9|0)==($195|0);
     if ($196) {
      HEAP32[$194>>2] = $R7$1;
      $cond54 = ($R7$1|0)==(0|0);
      if ($cond54) {
       $197 = 1 << $193;
       $198 = $197 ^ -1;
       $199 = HEAP32[((11208 + 4|0))>>2]|0;
       $200 = $199 & $198;
       HEAP32[((11208 + 4|0))>>2] = $200;
       break;
      }
     } else {
      $201 = HEAP32[((11208 + 16|0))>>2]|0;
      $202 = ($163>>>0)<($201>>>0);
      if ($202) {
       _abort();
       // unreachable;
      }
      $203 = (($163) + 16|0);
      $204 = HEAP32[$203>>2]|0;
      $205 = ($204|0)==($9|0);
      if ($205) {
       HEAP32[$203>>2] = $R7$1;
      } else {
       $206 = (($163) + 20|0);
       HEAP32[$206>>2] = $R7$1;
      }
      $207 = ($R7$1|0)==(0|0);
      if ($207) {
       break;
      }
     }
     $208 = HEAP32[((11208 + 16|0))>>2]|0;
     $209 = ($R7$1>>>0)<($208>>>0);
     if ($209) {
      _abort();
      // unreachable;
     }
     $210 = (($R7$1) + 24|0);
     HEAP32[$210>>2] = $163;
     $$sum19 = (($8) + 8)|0;
     $211 = (($mem) + ($$sum19)|0);
     $212 = HEAP32[$211>>2]|0;
     $213 = ($212|0)==(0|0);
     do {
      if (!($213)) {
       $214 = HEAP32[((11208 + 16|0))>>2]|0;
       $215 = ($212>>>0)<($214>>>0);
       if ($215) {
        _abort();
        // unreachable;
       } else {
        $216 = (($R7$1) + 16|0);
        HEAP32[$216>>2] = $212;
        $217 = (($212) + 24|0);
        HEAP32[$217>>2] = $R7$1;
        break;
       }
      }
     } while(0);
     $$sum20 = (($8) + 12)|0;
     $218 = (($mem) + ($$sum20)|0);
     $219 = HEAP32[$218>>2]|0;
     $220 = ($219|0)==(0|0);
     if (!($220)) {
      $221 = HEAP32[((11208 + 16|0))>>2]|0;
      $222 = ($219>>>0)<($221>>>0);
      if ($222) {
       _abort();
       // unreachable;
      } else {
       $223 = (($R7$1) + 20|0);
       HEAP32[$223>>2] = $219;
       $224 = (($219) + 24|0);
       HEAP32[$224>>2] = $R7$1;
       break;
      }
     }
    }
   }
  } while(0);
  $225 = $135 | 1;
  $226 = (($p$0) + 4|0);
  HEAP32[$226>>2] = $225;
  $227 = (($p$0) + ($135)|0);
  HEAP32[$227>>2] = $135;
  $228 = HEAP32[((11208 + 20|0))>>2]|0;
  $229 = ($p$0|0)==($228|0);
  if ($229) {
   HEAP32[((11208 + 8|0))>>2] = $135;
   STACKTOP = sp;return;
  } else {
   $psize$1 = $135;
  }
 } else {
  $230 = $114 & -2;
  HEAP32[$113>>2] = $230;
  $231 = $psize$0 | 1;
  $232 = (($p$0) + 4|0);
  HEAP32[$232>>2] = $231;
  $233 = (($p$0) + ($psize$0)|0);
  HEAP32[$233>>2] = $psize$0;
  $psize$1 = $psize$0;
 }
 $234 = $psize$1 >>> 3;
 $235 = ($psize$1>>>0)<(256);
 if ($235) {
  $236 = $234 << 1;
  $237 = ((11208 + ($236<<2)|0) + 40|0);
  $238 = HEAP32[11208>>2]|0;
  $239 = 1 << $234;
  $240 = $238 & $239;
  $241 = ($240|0)==(0);
  if ($241) {
   $242 = $238 | $239;
   HEAP32[11208>>2] = $242;
   $$sum16$pre = (($236) + 2)|0;
   $$pre = ((11208 + ($$sum16$pre<<2)|0) + 40|0);
   $$pre$phiZ2D = $$pre;$F16$0 = $237;
  } else {
   $$sum17 = (($236) + 2)|0;
   $243 = ((11208 + ($$sum17<<2)|0) + 40|0);
   $244 = HEAP32[$243>>2]|0;
   $245 = HEAP32[((11208 + 16|0))>>2]|0;
   $246 = ($244>>>0)<($245>>>0);
   if ($246) {
    _abort();
    // unreachable;
   } else {
    $$pre$phiZ2D = $243;$F16$0 = $244;
   }
  }
  HEAP32[$$pre$phiZ2D>>2] = $p$0;
  $247 = (($F16$0) + 12|0);
  HEAP32[$247>>2] = $p$0;
  $248 = (($p$0) + 8|0);
  HEAP32[$248>>2] = $F16$0;
  $249 = (($p$0) + 12|0);
  HEAP32[$249>>2] = $237;
  STACKTOP = sp;return;
 }
 $250 = $psize$1 >>> 8;
 $251 = ($250|0)==(0);
 if ($251) {
  $I18$0 = 0;
 } else {
  $252 = ($psize$1>>>0)>(16777215);
  if ($252) {
   $I18$0 = 31;
  } else {
   $253 = (($250) + 1048320)|0;
   $254 = $253 >>> 16;
   $255 = $254 & 8;
   $256 = $250 << $255;
   $257 = (($256) + 520192)|0;
   $258 = $257 >>> 16;
   $259 = $258 & 4;
   $260 = $259 | $255;
   $261 = $256 << $259;
   $262 = (($261) + 245760)|0;
   $263 = $262 >>> 16;
   $264 = $263 & 2;
   $265 = $260 | $264;
   $266 = (14 - ($265))|0;
   $267 = $261 << $264;
   $268 = $267 >>> 15;
   $269 = (($266) + ($268))|0;
   $270 = $269 << 1;
   $271 = (($269) + 7)|0;
   $272 = $psize$1 >>> $271;
   $273 = $272 & 1;
   $274 = $273 | $270;
   $I18$0 = $274;
  }
 }
 $275 = ((11208 + ($I18$0<<2)|0) + 304|0);
 $276 = (($p$0) + 28|0);
 $I18$0$c = $I18$0;
 HEAP32[$276>>2] = $I18$0$c;
 $277 = (($p$0) + 20|0);
 HEAP32[$277>>2] = 0;
 $278 = (($p$0) + 16|0);
 HEAP32[$278>>2] = 0;
 $279 = HEAP32[((11208 + 4|0))>>2]|0;
 $280 = 1 << $I18$0;
 $281 = $279 & $280;
 $282 = ($281|0)==(0);
 L199: do {
  if ($282) {
   $283 = $279 | $280;
   HEAP32[((11208 + 4|0))>>2] = $283;
   HEAP32[$275>>2] = $p$0;
   $284 = (($p$0) + 24|0);
   HEAP32[$284>>2] = $275;
   $285 = (($p$0) + 12|0);
   HEAP32[$285>>2] = $p$0;
   $286 = (($p$0) + 8|0);
   HEAP32[$286>>2] = $p$0;
  } else {
   $287 = HEAP32[$275>>2]|0;
   $288 = ($I18$0|0)==(31);
   if ($288) {
    $296 = 0;
   } else {
    $289 = $I18$0 >>> 1;
    $290 = (25 - ($289))|0;
    $296 = $290;
   }
   $291 = (($287) + 4|0);
   $292 = HEAP32[$291>>2]|0;
   $293 = $292 & -8;
   $294 = ($293|0)==($psize$1|0);
   L204: do {
    if ($294) {
     $T$0$lcssa = $287;
    } else {
     $295 = $psize$1 << $296;
     $K19$057 = $295;$T$056 = $287;
     while(1) {
      $303 = $K19$057 >>> 31;
      $304 = ((($T$056) + ($303<<2)|0) + 16|0);
      $299 = HEAP32[$304>>2]|0;
      $305 = ($299|0)==(0|0);
      if ($305) {
       break;
      }
      $297 = $K19$057 << 1;
      $298 = (($299) + 4|0);
      $300 = HEAP32[$298>>2]|0;
      $301 = $300 & -8;
      $302 = ($301|0)==($psize$1|0);
      if ($302) {
       $T$0$lcssa = $299;
       break L204;
      } else {
       $K19$057 = $297;$T$056 = $299;
      }
     }
     $306 = HEAP32[((11208 + 16|0))>>2]|0;
     $307 = ($304>>>0)<($306>>>0);
     if ($307) {
      _abort();
      // unreachable;
     } else {
      HEAP32[$304>>2] = $p$0;
      $308 = (($p$0) + 24|0);
      HEAP32[$308>>2] = $T$056;
      $309 = (($p$0) + 12|0);
      HEAP32[$309>>2] = $p$0;
      $310 = (($p$0) + 8|0);
      HEAP32[$310>>2] = $p$0;
      break L199;
     }
    }
   } while(0);
   $311 = (($T$0$lcssa) + 8|0);
   $312 = HEAP32[$311>>2]|0;
   $313 = HEAP32[((11208 + 16|0))>>2]|0;
   $314 = ($T$0$lcssa>>>0)<($313>>>0);
   if ($314) {
    _abort();
    // unreachable;
   }
   $315 = ($312>>>0)<($313>>>0);
   if ($315) {
    _abort();
    // unreachable;
   } else {
    $316 = (($312) + 12|0);
    HEAP32[$316>>2] = $p$0;
    HEAP32[$311>>2] = $p$0;
    $317 = (($p$0) + 8|0);
    HEAP32[$317>>2] = $312;
    $318 = (($p$0) + 12|0);
    HEAP32[$318>>2] = $T$0$lcssa;
    $319 = (($p$0) + 24|0);
    HEAP32[$319>>2] = 0;
    break;
   }
  }
 } while(0);
 $320 = HEAP32[((11208 + 32|0))>>2]|0;
 $321 = (($320) + -1)|0;
 HEAP32[((11208 + 32|0))>>2] = $321;
 $322 = ($321|0)==(0);
 if ($322) {
  $sp$0$in$i = ((11208 + 456|0));
 } else {
  STACKTOP = sp;return;
 }
 while(1) {
  $sp$0$i = HEAP32[$sp$0$in$i>>2]|0;
  $323 = ($sp$0$i|0)==(0|0);
  $324 = (($sp$0$i) + 8|0);
  if ($323) {
   break;
  } else {
   $sp$0$in$i = $324;
  }
 }
 HEAP32[((11208 + 32|0))>>2] = -1;
 STACKTOP = sp;return;
}
function runPostSets() {
 
}
function _memset(ptr, value, num) {
    ptr = ptr|0; value = value|0; num = num|0;
    var stop = 0, value4 = 0, stop4 = 0, unaligned = 0;
    stop = (ptr + num)|0;
    if ((num|0) >= 20) {
      // This is unaligned, but quite large, so work hard to get to aligned settings
      value = value & 0xff;
      unaligned = ptr & 3;
      value4 = value | (value << 8) | (value << 16) | (value << 24);
      stop4 = stop & ~3;
      if (unaligned) {
        unaligned = (ptr + 4 - unaligned)|0;
        while ((ptr|0) < (unaligned|0)) { // no need to check for stop, since we have large num
          HEAP8[(ptr)]=value;
          ptr = (ptr+1)|0;
        }
      }
      while ((ptr|0) < (stop4|0)) {
        HEAP32[((ptr)>>2)]=value4;
        ptr = (ptr+4)|0;
      }
    }
    while ((ptr|0) < (stop|0)) {
      HEAP8[(ptr)]=value;
      ptr = (ptr+1)|0;
    }
    return (ptr-num)|0;
}
function _strlen(ptr) {
    ptr = ptr|0;
    var curr = 0;
    curr = ptr;
    while (((HEAP8[(curr)])|0)) {
      curr = (curr + 1)|0;
    }
    return (curr - ptr)|0;
}
function _memcpy(dest, src, num) {
    dest = dest|0; src = src|0; num = num|0;
    var ret = 0;
    if ((num|0) >= 4096) return _emscripten_memcpy_big(dest|0, src|0, num|0)|0;
    ret = dest|0;
    if ((dest&3) == (src&3)) {
      while (dest & 3) {
        if ((num|0) == 0) return ret|0;
        HEAP8[(dest)]=((HEAP8[(src)])|0);
        dest = (dest+1)|0;
        src = (src+1)|0;
        num = (num-1)|0;
      }
      while ((num|0) >= 4) {
        HEAP32[((dest)>>2)]=((HEAP32[((src)>>2)])|0);
        dest = (dest+4)|0;
        src = (src+4)|0;
        num = (num-4)|0;
      }
    }
    while ((num|0) > 0) {
      HEAP8[(dest)]=((HEAP8[(src)])|0);
      dest = (dest+1)|0;
      src = (src+1)|0;
      num = (num-1)|0;
    }
    return ret|0;
}

// EMSCRIPTEN_END_FUNCS

  
  function dynCall_iiii(index,a1,a2,a3) {
    index = index|0;
    a1=a1|0; a2=a2|0; a3=a3|0;
    return FUNCTION_TABLE_iiii[index&1](a1|0,a2|0,a3|0)|0;
  }


  function dynCall_vii(index,a1,a2) {
    index = index|0;
    a1=a1|0; a2=a2|0;
    FUNCTION_TABLE_vii[index&1](a1|0,a2|0);
  }

function b0(p0,p1,p2) { p0 = p0|0;p1 = p1|0;p2 = p2|0; nullFunc_iiii(0);return 0; }
  function b1(p0,p1) { p0 = p0|0;p1 = p1|0; nullFunc_vii(1); }
  // EMSCRIPTEN_END_FUNCS
  var FUNCTION_TABLE_iiii = [b0,_zcalloc];
  var FUNCTION_TABLE_vii = [b1,_zcfree];

  return { _malloc: _malloc, _strlen: _strlen, _free: _free, _memset: _memset, _cromon_inflate: _cromon_inflate, _memcpy: _memcpy, runPostSets: runPostSets, stackAlloc: stackAlloc, stackSave: stackSave, stackRestore: stackRestore, setThrew: setThrew, setTempRet0: setTempRet0, setTempRet1: setTempRet1, setTempRet2: setTempRet2, setTempRet3: setTempRet3, setTempRet4: setTempRet4, setTempRet5: setTempRet5, setTempRet6: setTempRet6, setTempRet7: setTempRet7, setTempRet8: setTempRet8, setTempRet9: setTempRet9, dynCall_iiii: dynCall_iiii, dynCall_vii: dynCall_vii };
})
// EMSCRIPTEN_END_ASM
({ "Math": Math, "Int8Array": Int8Array, "Int16Array": Int16Array, "Int32Array": Int32Array, "Uint8Array": Uint8Array, "Uint16Array": Uint16Array, "Uint32Array": Uint32Array, "Float32Array": Float32Array, "Float64Array": Float64Array }, { "abort": abort, "assert": assert, "asmPrintInt": asmPrintInt, "asmPrintFloat": asmPrintFloat, "min": Math_min, "nullFunc_iiii": nullFunc_iiii, "nullFunc_vii": nullFunc_vii, "invoke_iiii": invoke_iiii, "invoke_vii": invoke_vii, "_fflush": _fflush, "_abort": _abort, "___setErrNo": ___setErrNo, "_sbrk": _sbrk, "_time": _time, "_emscripten_memcpy_big": _emscripten_memcpy_big, "_sysconf": _sysconf, "___errno_location": ___errno_location, "STACKTOP": STACKTOP, "STACK_MAX": STACK_MAX, "tempDoublePtr": tempDoublePtr, "ABORT": ABORT, "NaN": NaN, "Infinity": Infinity }, buffer);
var _malloc = Module["_malloc"] = asm["_malloc"];
var _strlen = Module["_strlen"] = asm["_strlen"];
var _free = Module["_free"] = asm["_free"];
var _memset = Module["_memset"] = asm["_memset"];
var _cromon_inflate = Module["_cromon_inflate"] = asm["_cromon_inflate"];
var _memcpy = Module["_memcpy"] = asm["_memcpy"];
var runPostSets = Module["runPostSets"] = asm["runPostSets"];
var dynCall_iiii = Module["dynCall_iiii"] = asm["dynCall_iiii"];
var dynCall_vii = Module["dynCall_vii"] = asm["dynCall_vii"];

Runtime.stackAlloc = function(size) { return asm['stackAlloc'](size) };
Runtime.stackSave = function() { return asm['stackSave']() };
Runtime.stackRestore = function(top) { asm['stackRestore'](top) };


// Warning: printing of i64 values may be slightly rounded! No deep i64 math used, so precise i64 code not included
var i64Math = null;

// === Auto-generated postamble setup entry stuff ===

if (memoryInitializer) {
  if (ENVIRONMENT_IS_NODE || ENVIRONMENT_IS_SHELL) {
    var data = Module['readBinary'](memoryInitializer);
    HEAPU8.set(data, STATIC_BASE);
  } else {
    addRunDependency('memory initializer');
    Browser.asyncLoad(memoryInitializer, function(data) {
      HEAPU8.set(data, STATIC_BASE);
      removeRunDependency('memory initializer');
    }, function(data) {
      throw 'could not load memory initializer ' + memoryInitializer;
    });
  }
}

function ExitStatus(status) {
  this.name = "ExitStatus";
  this.message = "Program terminated with exit(" + status + ")";
  this.status = status;
};
ExitStatus.prototype = new Error();
ExitStatus.prototype.constructor = ExitStatus;

var initialStackTop;
var preloadStartTime = null;
var calledMain = false;

dependenciesFulfilled = function runCaller() {
  // If run has never been called, and we should call run (INVOKE_RUN is true, and Module.noInitialRun is not false)
  if (!Module['calledRun'] && shouldRunNow) run();
  if (!Module['calledRun']) dependenciesFulfilled = runCaller; // try this again later, after new deps are fulfilled
}

Module['callMain'] = Module.callMain = function callMain(args) {
  assert(runDependencies == 0, 'cannot call main when async dependencies remain! (listen on __ATMAIN__)');
  assert(__ATPRERUN__.length == 0, 'cannot call main when preRun functions remain to be called');

  args = args || [];

  ensureInitRuntime();

  var argc = args.length+1;
  function pad() {
    for (var i = 0; i < 4-1; i++) {
      argv.push(0);
    }
  }
  var argv = [allocate(intArrayFromString("/bin/this.program"), 'i8', ALLOC_NORMAL) ];
  pad();
  for (var i = 0; i < argc-1; i = i + 1) {
    argv.push(allocate(intArrayFromString(args[i]), 'i8', ALLOC_NORMAL));
    pad();
  }
  argv.push(0);
  argv = allocate(argv, 'i32', ALLOC_NORMAL);

  initialStackTop = STACKTOP;

  try {

    var ret = Module['_main'](argc, argv, 0);


    // if we're not running an evented main loop, it's time to exit
    if (!Module['noExitRuntime']) {
      exit(ret);
    }
  }
  catch(e) {
    if (e instanceof ExitStatus) {
      // exit() throws this once it's done to make sure execution
      // has been stopped completely
      return;
    } else if (e == 'SimulateInfiniteLoop') {
      // running an evented main loop, don't immediately exit
      Module['noExitRuntime'] = true;
      return;
    } else {
      if (e && typeof e === 'object' && e.stack) Module.printErr('exception thrown: ' + [e, e.stack]);
      throw e;
    }
  } finally {
    calledMain = true;
  }
}




function run(args) {
  args = args || Module['arguments'];

  if (preloadStartTime === null) preloadStartTime = Date.now();

  if (runDependencies > 0) {
    Module.printErr('run() called, but dependencies remain, so not running');
    return;
  }

  preRun();

  if (runDependencies > 0) return; // a preRun added a dependency, run will be called later
  if (Module['calledRun']) return; // run may have just been called through dependencies being fulfilled just in this very frame

  function doRun() {
    if (Module['calledRun']) return; // run may have just been called while the async setStatus time below was happening
    Module['calledRun'] = true;

    ensureInitRuntime();

    preMain();

    if (ENVIRONMENT_IS_WEB && preloadStartTime !== null) {
      Module.printErr('pre-main prep time: ' + (Date.now() - preloadStartTime) + ' ms');
    }

    if (Module['_main'] && shouldRunNow) {
      Module['callMain'](args);
    }

    postRun();
  }

  if (Module['setStatus']) {
    Module['setStatus']('Running...');
    setTimeout(function() {
      setTimeout(function() {
        Module['setStatus']('');
      }, 1);
      if (!ABORT) doRun();
    }, 1);
  } else {
    doRun();
  }
}
Module['run'] = Module.run = run;

function exit(status) {
  ABORT = true;
  EXITSTATUS = status;
  STACKTOP = initialStackTop;

  // exit the runtime
  exitRuntime();

  // TODO We should handle this differently based on environment.
  // In the browser, the best we can do is throw an exception
  // to halt execution, but in node we could process.exit and
  // I'd imagine SM shell would have something equivalent.
  // This would let us set a proper exit status (which
  // would be great for checking test exit statuses).
  // https://github.com/kripken/emscripten/issues/1371

  // throw an exception to halt the current execution
  throw new ExitStatus(status);
}
Module['exit'] = Module.exit = exit;

function abort(text) {
  if (text) {
    Module.print(text);
    Module.printErr(text);
  }

  ABORT = true;
  EXITSTATUS = 1;

  var extra = '';

  throw 'abort() at ' + stackTrace() + extra;
}
Module['abort'] = Module.abort = abort;

// {{PRE_RUN_ADDITIONS}}

if (Module['preInit']) {
  if (typeof Module['preInit'] == 'function') Module['preInit'] = [Module['preInit']];
  while (Module['preInit'].length > 0) {
    Module['preInit'].pop()();
  }
}

// shouldRunNow refers to calling main(), not run().
var shouldRunNow = true;
if (Module['noInitialRun']) {
  shouldRunNow = false;
}


run();

// {{POST_RUN_ADDITIONS}}






// {{MODULE_ADDITIONS}}



