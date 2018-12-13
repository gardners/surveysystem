/**
 * @module Utils
 */

/**
 * Checks if a value is a native Array
 * @param {*} v value
 * @returns {boolean}
 */
const isArray = function(v) {
    return Object.prototype.toString.call(v) === '[object Array]';
};

/**
 * Checks if a value is a native Object
 * @param {*} v value
 * @returns {boolean}
 */
const isObject = function(v) {
    return Object.prototype.toString.call(v) === '[object Object]';
};

/**
 * Checks if a value is an empty Object {}
 * @param {object} obj
 * @returns {boolean}
 */
const isEmptyObject = function(obj) {
    for(let key in obj) {
        if(obj.hasOwnProperty(key)) {
            return false;
        }
    }
    return true;
};

/**
 * Checks if a value is scalar
 * @param {*} v value
 * @returns {boolean}
 */
const isScalar = function(v) {
    const type = typeof v;
    return v === null || ['string', 'number', 'boolean'].indexOf(type) > -1;
};

const DirtyJson = {
    coerceArray(v = null) {
        if (!v) {
            return [];
        }

        if (isScalar(v)) {
            return [v];
        }

        if (isObject(v) && !isEmptyObject(v)) {
            return [v];
        }

        return (isArray(v)) ? v : [];
    },

    get(v, key, r = null) {
        return (v && typeof v[key] !== 'undefined') ? v[key] : r;
    },

};

export {
    isScalar,
    DirtyJson,
};
