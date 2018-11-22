/**
 * @module storage/LocalStorage
 */
const LocalStorage = {

    /**
     * Get stored value for key
     * @param {string} key
     * @returns {(*|null)} deserialized JSON or null if value was not found
     */
    get: function(key) {
        const val = localStorage.getItem(key);
        if (val) {
            try {
                return JSON.parse(val);
            } catch (e) {
                return null;
            }
        }
    },

    /**
     * Set or replace stored value for key. The value will be serialized.
     * @param {string} key
     * @param {*} value
     * @returns {void}
     */
    set: function(key, value) {
        const json = JSON.stringify(value);
        localStorage.setItem(key, json);
    },

    /**
     * Delete stored key
     * @param {string} key
     * @returns {void}
     */
    delete: function(key) {
        localStorage.removeItem(key);
    },
};

export default LocalStorage;
