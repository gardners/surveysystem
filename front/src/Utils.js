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
        if (obj.hasOwnProperty(key)) {
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
    return str.replace(/([A-Z])/g, ' $1').trim();
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

/**
 * Removes theescaping of forward slashes in an kcgi generated string
 * @TODO !Note: this can be removed should afromentioned issue be resolved in the backend
 * @see https://kristaps.bsd.lv/kcgi/kcgijson.3.html, kjson_putstringp()
 * @see https://github.com/gardners/surveysystem/issues/119
 * @param {string} str
 *
 * @returns {string}
 */
const sanitizeKcgiJsonString = function(str) {
    return str.replace('\\/', '/');
};

////
// time helpers
////

const DaySeconds = 86400; // 24 hours

/**
 * date timestamp parser utils
 */

// m: 00:00
// m: 04:00
// s: 08:00
// s: 12:00
// s: 16:00
// m: 20:00
// m: 24:00

/**
 * Helper function for parsing time values from seconds
 * @param {seconds}
 *
 * @returns {object}
 */
const parseDayTime = function(seconds) {
    const date = new Date(seconds * 1000);
    const hours24 = date.getUTCHours(); // 0 - 23
    const mins = date.getUTCMinutes();

    let hours = (hours24 > 12) ? hours24 - 12 : hours24;
    let ampm = (hours24 < 12) ? 'am': 'pm';

    if(seconds === DaySeconds) {
        hours = 12;
        ampm = 'pm';
    }

    return {
        days: Math.floor(seconds / DaySeconds),
        hours24,
        hours,
        mins,
        ampm,
    };
};

const formatDayTimeDiff = function(fromSeconds, toSeconds, withSeconds = false) {
    const from = new Date(fromSeconds * 1000);
    const to = new Date(toSeconds * 1000);

    const diff = to - from;

    let d = diff / 1000;
    const secs = Math.floor(d % 60);

    d = d / 60;
    const mins = Math.floor(d % 60);

    d = d / 60;
    const hours = Math.floor(d % 24);
    const days = Math.floor(d / 24);

    let r = (diff === 0) ? '0' : (diff < 0) ? '- ' : '+ ';

    if(days) {
        r += Math.abs(days);
        r += (days === 1) ? ' day ': ' days ';
    }

    if(hours) {
        r += Math.abs(hours);
        r += (hours === 1) ? ' hour ': ' hours ';
    }

    if(mins) {
        r += Math.abs(mins);
        r += (mins === 1) ? ' minute ': ' minutes ';
    }

    if (withSeconds && secs) {
        r += Math.abs(secs);
        r += (secs === 1) ? ' second ': ' seconds ';
    }

    return r.trim();
};

const formatDayTime = function(seconds) {
    const dt = parseDayTime(seconds);

    const hh = (dt.hours < 10) ? `0${dt.hours}`: dt.hours;
    const mm = (dt.mins < 10 ) ? `0${dt.mins}`: dt.mins

    return `${hh}:${mm} ${dt.ampm}`;
};

const setDaytimeDate = function(hours12, minutes, ampm) {
    const hours = (ampm === 'pm' && hours12 <= 12) ? hours12 + 12 : hours12;

    const YYYY = '1970';
    const MM = '01';
    const DD = '01';
    const HH = (hours < 10) ? `0${hours}` : hours;
    const mm = (minutes < 10) ? `0${minutes}` : minutes;
    const ss = '00';
    const sss = '000';

    // 1970-01-01THH:mm:ss.sssZ
    return new Date(`${YYYY}-${MM}-${DD}T${HH}:${mm}:${ss}.${sss}Z`);
};

const serializeParams = function(params) {
    return Object.keys(params).map((key) => {
        return encodeURIComponent(key) + '=' + encodeURIComponent(params[key]);

    }).join('&');
};

const isoDateToLocale = function(str) {

    // work around the various Date() pitifalls
    let d = NaN;
    try {
        d = new Date(str);
    } catch (e) {
        // nothing
    }

    return (!d || isNaN(d)) ? '' : d.toLocaleString();
};

export {
    isScalar,
    DirtyJson,
    camelToNormal,
    isArray,
    isObject,
    addClassNames,
    sanitizeKcgiJsonString,
    DaySeconds,
    parseDayTime,
    formatDayTime,
    formatDayTimeDiff,
    setDaytimeDate,
    serializeParams,
    isoDateToLocale,
};
