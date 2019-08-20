import PropTypes from 'prop-types';

const {
    PUBLIC_URL,
    REACT_APP_SURVEY_LIST,
} = process.env;

const coerce = function(id, data) {
     return {
        id,
        name: data.name,
        title: data.title || '',
        description: data.description || '',
        organisation: data.organisation || '',
        email: data.email || '',
        phone: data.phone || '',
        pages: data.pages || [],
    };
};

const getById = function(id) {
    return fetch(`${PUBLIC_URL}/surveys/${id}/survey.json`)
    .then(response => response.json())
    .then(data => coerce(id, data));
};

const getAll = function() {
    const surveyIds = REACT_APP_SURVEY_LIST.split(',').map(name => name.trim());
    return Promise.all(surveyIds.map(id => getById(id)));
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
        pages: PropTypes.array,
    });
};

export default { getAll, getById, itemPropTypes };
