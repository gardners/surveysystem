
const LocalStorage = {
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

    set: function(key, value) {
        const json = JSON.stringify(value);
        return localStorage.setItem(key, json);
    },

    delete: function(key) {
        localStorage.removeItem(key);
    },
};

export default LocalStorage;
