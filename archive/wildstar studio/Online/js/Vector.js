var Vector2 = function (x, y) {
    x = x || 0.0;
    y = y || 0.0;

    this.x = x;
    this.y = y;
}

Vector2.prototype.multiply = function (val) {
    this.x *= val;
    this.y *= val;

    return this;
}

Vector2.prototype.length = function () {
    return Math.sqrt(this.x * this.x + this.y * this.y);
}

Vector2.prototype.lengthSquared = function () {
    return this.x * this.x + this.y * this.y;
}

Vector2.prototype.subtract = function (vector) {
    this.x -= vector.x;
    this.y -= vector.y;

    return this;
}

Vector2.prototype.add = function (vector) {
    this.x += vector.x;
    this.y += vector.y;
}

Vector2.multiply = function (v1, val) {
    return new Vector2(v1.x * val, v1.y * val);
}

Vector2.subtract = function (v1, v2) {
    return new Vector2(v1.x - v2.x, v1.y - v2.y);
}

Vector2.add = function (v1, v2) {
    return new Vector2(v1.x + v2.x, v1.y + v2.y);
}

var Vector3 = function (x, y, z) {
    x = x || 0.0;
    y = y || 0.0;
    z = z || 0.0;

    this.x = x;
    this.y = y;
    this.z = z;
}

Vector3.prototype.length = function () {
    return Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
}

Vector3.prototype.normalize = function () {
    var len = this.length();
    if (Math.abs(len) < 0.000001) {
        throw new TypeError('Cannot normalize vector: Length near singular');
    }

    this.x /= len;
    this.y /= len;
    this.z /= len;

    return this;
}

Vector3.prototype.normalized = function () {
    return new Vector3(this.x, this.y, this.z).normalize();
}

Vector3.prototype.takeMin = function (vector) {
    if (vector.x < this.x) {
        this.x = vector.x;
    }

    if (vector.y < this.y) {
        this.y = vector.y;
    }

    if (vector.z < this.z) {
        this.z = vector.z;
    }
}

Vector3.prototype.takeMax = function (vector) {
    if (vector.x > this.x) {
        this.x = vector.x;
    }

    if (vector.y > this.y) {
        this.y = vector.y;
    }

    if (vector.z > this.z) {
        this.z = vector.z;
    }
}

Vector3.prototype.lengthSquared = function () {
    return this.x * this.x + this.y * this.y + this.z * this.z;
}

Vector3.prototype.dot = function (vector) {
    return this.x * vector.x + this.y * vector.y + this.z * vector.z;
}

Vector3.prototype.cross = function (vector) {
    return new Vector3(
        this.y * vector.z - this.z * vector.y,
        this.z * vector.x - this.x * vector.z,
        this.x * vector.y - this.y * vector.x
    );
}

Vector3.prototype.multiply = function (val) {
    this.x *= val;
    this.y *= val;
    this.z *= val;

    return this;
}

Vector3.prototype.divide = function (val) {
    this.x /= val;
    this.y /= val;
    this.z /= val;

    return this;
}

Vector3.cross = function (v1, v2) {
    return new Vector3(
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    );
}

Vector3.multiply = function (v, val) {
    return new Vector3(v.x * val, v.y * val, v.z * val);
}

Vector3.divide = function (v, val) {
    return new Vector3(v.x / val, v.y / val, v.z / val);
}

Vector3.negate = function (v) {
    return new Vector3(-v.x, -v.y, -v.z);
}

Vector3.UnitX = new Vector3(1, 0, 0);
Vector3.UnitY = new Vector3(0, 1, 0);
Vector3.UnitZ = new Vector3(0, 0, 1);