import PropTypes from 'prop-types';

export default {

    propTypes: function (withChoices = false) {
        return PropTypes.shape({
            id: PropTypes.string.isRequired,
            name: PropTypes.string.isRequired,
            title: PropTypes.string.isRequired,
            title_text: PropTypes.string.isRequired,
            type: PropTypes.string.isRequired,
            choices: (!withChoices) ? PropTypes.array : PropTypes.array.isRequired,
            unit: PropTypes.string.isRequired,
        });
    }

};
