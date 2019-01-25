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

/**
 * @module Utils/Dirtyjson
 */
const DirtyJson = {

    /**
     * Coerces value into Array
     * @param {*} v value
     * @returns {Array}
     */
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

    /**
     * Gets property value from Object or provides return value
     *  - non recursive!
     * @param {object} v lookup object
     * @param {string} key Property
     * @param {string} [r] optional return value if property not found
     * @returns {*|null} value or r or null
     */
    get(v, key, r = null) {
        return (v && typeof v[key] !== 'undefined') ? v[key] : r;
    },

};

/**
 * Converts a CamelCase String into Normal Case
 * @param {string} str
 * @returns {string}
 */
const camelToNormal = function(str) {
    return str.replace(/([A-Z])/g, ' $1');
};

/**
 * Build dom element className string from chunks
 * due to coherent window.classList support across IE
 * @param {...string} classNames class names
 *
 * @returns {string}
 */
const addClassNames = function(...classNames) {
    return classNames.reduce((acc, value) => {
        return (typeof value === 'string' && value) ? `${acc} ${value}`.trim() : acc;
    }, '');
};

export {
    isScalar,
    DirtyJson,
    camelToNormal,
    isArray,
    addClassNames,
};
