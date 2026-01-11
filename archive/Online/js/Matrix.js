var Matrix = function () {
    this.values = new Float32Array(16);
    this.setIdentity();
}

Matrix.prototype.setIdentity = function () {
    for (var i = 0; i < 4; ++i) {
        for (var j = 0; j < 4; ++j) {
            this.values[i * 4 + j] = (i == j) ? 1 : 0;
        }
    }
}

Matrix.prototype.transpose = function () {
    for (var i = 0; i < 4; ++i) {
        for (var j = 0; j < 4; ++j) {
            var tmp = this.values[j * 4 + i];
            this.values[j * 4 + i] = this.values[i * 4 + j];
            this.values[i * 4 + j] = tmp;
        }
    }

    return this;
}

Matrix.scale = function (x, y, z) {
    var ret = new Matrix();
    ret.values[0] = x;
    ret.values[5] = y;
    ret.values[10] = z;

    return ret;
}

Matrix.translation = function (x, y, z) {
    var ret = new Matrix();
    ret.values[3] = x;
    ret.values[7] = y;
    ret.values[11] = z;

    return ret;
}

Matrix.rotation = function (angle, axis) {
    angle = (angle * Math.PI) / 180.0;
    var cosv = Math.cos(angle);
    var sinv = Math.sin(angle);
    var ux = axis.x;
    var uy = axis.y;
    var uz = axis.z;
    var ux2 = ux * ux;
    var uy2 = uy * uy;
    var uz2 = uz * uz;

    var ret = new Matrix();
    ret.values[0] = cosv + ux2 * (1.0 - cosv);
    ret.values[1] = ux * uy * (1 - cosv) - uz * sinv;
    ret.values[2] = ux * uz * (1 - cosv) + uy * sinv;
    ret.values[4] = uy * ux * (1 - cosv) + uz * sinv;
    ret.values[5] = cosv + uy2 * (1 - cosv);
    ret.values[6] = uy * uz * (1 - cosv) - ux * sinv;
    ret.values[8] = uz * ux * (1 - cosv) - uy * sinv;
    ret.values[9] = uz * uy * (1 - cosv) + ux * sinv;
    ret.values[10] = cosv + uz2 * (1 - cosv);
}

Matrix.rotationAxis = function (axis) {
    return Matrix.multiply(Matrix.multiply(Matrix.rotation(axis.x, Vector3.UnitX), Matrix.rotation(axis.y, Vector3.UnitY)), Matrix.rotation(axis.z, Vector3.UnitZ));
}

Matrix.perspective = function (fovy, aspect, zNear, zFar) {
    var ret = new Matrix();
    var top = zNear * Math.tan(fovy * Math.PI / 360.0);
    var bottom = -top;
    var left = bottom * aspect;
    var right = top * aspect;

    ret.values[0] = 2.0 * zNear / (right - left);
    ret.values[3] = (right + left) / (right - left);
    ret.values[6] = 2.0 * zNear / (top - bottom);
    ret.values[7] = (top + bottom) / (top - bottom);
    ret.values[10] = -(zFar + zNear) / (zFar - zNear);
    ret.values[11] = -2.0 * zFar * zNear / (zFar - zNear);
    ret.values[14] = -1;
    ret.values[15] = 0.0;

    return ret;
}

Matrix.ortho = function (left, top, right, bottom, zNear, zFar) {
    var ret = new Matrix();

    var lr = 1 / (left - right),
    bt = 1 / (bottom - top),
    nf = 1 / (zNear - zFar);
    ret.values[0] = -2 * lr;
    ret.values[1] = 0;
    ret.values[2] = 0;
    ret.values[3] = 0;
    ret.values[4] = 0;
    ret.values[5] = -2 * bt;
    ret.values[6] = 0;
    ret.values[7] = 0;
    ret.values[8] = 0;
    ret.values[9] = 0;
    ret.values[10] = 2 * nf;
    ret.values[11] = 0;
    ret.values[12] = (left + right) * lr;
    ret.values[13] = (top + bottom) * bt;
    ret.values[14] = (zFar + zNear) * nf;
    ret.values[15] = 1;

    return ret;
}

Matrix.lookAt = function (eye, at, up) {
    var forward = Vector3.subtract(eye, at);
    forward.normalize();
    var side = Vector3.negate(forward.cross(up));
    side.normalize();

    var ret = new Matrix();
    var floats = new Float32Array([
        side.x, up.x, forward.x, 0,
        side.y, up.y, forward.y, 0,
        side.z, up.z, forward.z, 0,
        -side.dot(eye), -up.dot(eye), -forward.dot(eye)
    ]);

    ret.values.set(floats);

    return ret;
}

Matrix.transposed = function (matrix) {
    var ret = new Matrix();
    for (var i = 0; i < 16; ++i) {
        ret.values[i] = matrix.values[i];
    }

    ret.transpose();
    return ret;
}

Matrix.multiply = function (matrix, vectorOrMatrix) {
    if (vectorOrMatrix instanceof Vector3) {
        var ret = new Vector3();
        var rhs = vectorOrMatrix;
        ret.x = rhs.x * matrix.values[0] + rhs.y * matrix.values[1] + rhs.z * matrix.values[2];
        ret.y = rhs.x * matrix.values[4] + rhs.y * matrix.values[5] + rhs.z * matrix.values[6];
        ret.z = rhs.x * matrix.values[8] + rhs.y * matrix.values[9] + rhs.z * matrix.values[10];

        var w = 1.0 / (rhs.x * matrix.values[12] + rhs.y * matrix.values[13] + rhs.z * matrix.values[14]);
        ret.x *= w;
        ret.y *= w;
        ret.z *= w;

        return ret;
    } else if (vectorOrMatrix instanceof Matrix) {
        var ret = new Matrix();
        var rhs = vectorOrMatrix;

        for (var i = 0; i < 4; ++i) {
            for (var j = 0; j < 4; ++j) {
                ret.values[i * 4 + j] =
                    matrix.values[i * 4] * rhs[j] +
                    matrix.values[i * 4 + 1] * rhs[4 + j] +
                    matrix.values[i * 4 + 2] * rhs[8 + j] +
                    matrix.values[i * 4 + 3] * rhs[12 + j];
            }
        }

        return ret;
    }
}