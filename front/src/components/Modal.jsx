import React from 'react';
import PropTypes from 'prop-types';

import ReactModal from 'react-modal';

import './Modal.scss';

const customStyles = {
    content : {
        top: '50%',
        left: '50%',
        right: 'auto',
        bottom: 'auto',
        marginRight: '-50%',
        transform: 'translate(-50%, -50%)',

        minWidth: '60%',
        /* @see node_modules/bootstrap/scss/bootstrap*/
        padding: '0', // $modal-inner-padding
        borderRadius: '.3rem', //$modal-content-border-radius
        border: '1px solid rgba(0, 0, 0, .2)' // $modal-content-border-color, $modal-content-border-width
    },
    overlay: {
        backgroundColor: 'rgba(0, 0, 0, 0.75)'
    }
};

ReactModal.setAppElement('#root');

/**
 * @see http://reactcommunity.org/react-modal/#usage
 */
class Modal extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            open: false
        };
    }

    openModal(e) {
        e && e.preventDefault();
        this.setState({
            open: true
        });
    }

    afterOpenModal() {
        const { onAfterOpen } = this.props;
        if (typeof onAfterOpen === 'function') {
            onAfterOpen();
        }
    }

    closeModal(e) {
        e && e.preventDefault();

        const { onRequestClose } = this.props;
        if (typeof onRequestClose === 'function') {
            onRequestClose();
        }

        this.setState({
            open: false
        });
    }

    render() {
        const { title, shouldCloseOnOverlayClick, buttonText, buttonClassName, children } = this.props;
        const { open } = this.state;

        return (
            <React.Fragment>
                <button className={ buttonClassName } onClick={ this.openModal.bind(this) }>{ buttonText() }</button>
                <ReactModal
                    closeTimeoutMS={ 500 }
                    isOpen={ open }
                    onAfterOpen={ this.afterOpenModal.bind(this) }
                    onRequestClose={ this.closeModal.bind(this) }
                    shouldCloseOnOverlayClick={ shouldCloseOnOverlayClick }
                    style={ customStyles }
                    contentLabel={ title }
                >
                    <div className="modal-header">
                        { title && <h5 className="modal-title">{ title }</h5> }
                        <button  onClick={ this.closeModal.bind(this) } type="button" className="close" data-dismiss="modal" aria-label="Close">
                            <span aria-hidden="true">&times;</span>
                        </button>
                    </div>
                    <div className="modal-body">
                        { children }
                    </div>
                    <div className="modal-footer">
                        <button onClick={ this.closeModal.bind(this) } type="button" className="btn btn-secondary">close</button>
                    </div>
                </ReactModal>
            </React.Fragment>
        );
    }
}

Modal.defaultProps = {
    title: '',
    subtitle: '',
    buttonClassName: 'btn btn-primary btn-sm',
    shouldCloseOnOverlayClick: true,
    shouldCloseOnEsc: true,
    onAfterOpen: function() {},
    onRequestClose: function() {},
};

Modal.propTypes = {
    buttonText: PropTypes.func.isRequired,
    buttonClassName: PropTypes.string,
    title: PropTypes.string,
    shouldCloseOnOverlayClick: PropTypes.bool,
    shouldCloseOnEsc: PropTypes.bool,
    onAfterOpen: PropTypes.func,
    onRequestClose: PropTypes.func,
};

export default Modal;
