/**
 * @module conf
 * loads configuration according to NODE_ENV
 * @see https://facebook.github.io/create-react-app/docs/adding-custom-environment-variables
 * This requires commonjs module loader (no dynamic imports), no worries webpack takes care of that
 */

const config = require(`./${process.env.NODE_ENV}.config.js`);
module.exports = config;
