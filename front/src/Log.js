/**
 * @module Log
 * Simple console.log wrapper
 * output disabled in production mode
 */
import { isScalar } from './Utils';

const toConsole = (process.env.NODE_ENV !== 'production');


const add = function(severity, msg, store) {
    const message = (msg === undefined || isScalar(msg)) ? msg : JSON.stringify(msg);
    store.push({
        severity,
        message,
    });
};

/**
 * @class
 * @classdesc Log store
 */
class Log {
    /**
     * Create a Log store
     * @return {void}
     */
    constructor() {
        this.messages = [];
        this.subscribers = {};
    }

    /**
     * Register a subscriber callback
     * @param {string} id
     * @param {function} callback
     *
     * @return {void}
     */
    subscribe(id, callback) {
        this.subscribers[id] = callback;
    }

    /**
     * Remove a subscriber callback
     * @param {string} id
     * @param {function} callback
     *
     * @return {void}
     */
    unsubscribe(id) {
        delete (this.subscribers[id]);
    }

    /**
     * Trigger registered subscriber callbacks
     *
     * @return {void}
     */
    invokeSubscribers() {
        Object.keys(this.subscribers).forEach((id) => {
            this.subscribers[id](this.messages);
        });
    }

    /**
     * Log a message
     * @param {mixed} message
     * @return {void}
     */
    log(message) {
        add('log', message, this.messages);
        this.invokeSubscribers();
        if (toConsole) {
            console.log(message);
        }
    }

    /**
     * Log an error
     * @param {mixed} message
     * @return {void}
     */
    error(message) {
        add('error', message, this.messages);
        this.invokeSubscribers();
        if (toConsole) {
            console.error(message);
        }
    }

    /**
     * Log a warning
     * @param {mixed} message
     * @return {void}
     */
    warn(message) {
        add('warn', message, this.messages);
        this.invokeSubscribers();
        if (toConsole) {
            console.warn(message);
        }
    }

    /**
     * Log a debug message
     * @param {mixed} message
     * @return {void}
     */
    debug(message) {
        add('debug', message, this.messages);
        if (toConsole) {
            console.debug(message);
        }
    }
}

export default new Log();
