import PropTypes from 'prop-types';

const coerce = function(id, data) {
     return {
        id,
        name: data.name,
        title: data.title || '',
        description: data.description || '',
        organisation: data.organisation || '',
        email: data.email || '',
    };
};

const getById = function(id) {
    return import(`./config-surveys/${id}/survey.json`)
    .then(data => coerce(id, data));
};

const getAll = function(ids) {
    return Promise.all(ids.map(id => getById(id)));
};


/**
* Get Proptypes schema
* @returns {PropTypes}
*/
const itemPropTypes = function () {
    return PropTypes.shape({
        id: PropTypes.string,
        name: PropTypes.string,
        title: PropTypes.string,
        description: PropTypes.string,
        organisation: PropTypes.string,
        email: PropTypes.string,
    });
};

export default { getAll, getById, itemPropTypes };