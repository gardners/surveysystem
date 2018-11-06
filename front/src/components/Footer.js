import React, { Component } from 'react';


class Footer extends Component {
    render() {
        return (
            <footer class="page-footer font-small special-color-dark pt-4">

                <div class="container">


                    <ul class="list-unstyled list-inline text-center">
                        <li class="list-inline-item">
                            <a class="btn-floating btn-fb mx-1">
                                <i class="fa fa-facebook"> </i>
                            </a>
                        </li>
                        <li class="list-inline-item">
                            <a class="btn-floating btn-tw mx-1">
                                <i class="fa fa-twitter"> </i>
                            </a>
                        </li>
                        <li class="list-inline-item">
                            <a class="btn-floating btn-gplus mx-1">
                                <i class="fa fa-google-plus"> </i>
                            </a>
                        </li>
                        <li class="list-inline-item">
                            <a class="btn-floating btn-li mx-1">
                                <i class="fa fa-linkedin"> </i>
                            </a>
                        </li>
                        <li class="list-inline-item">
                            <a class="btn-floating btn-dribbble mx-1">
                                <i class="fa fa-dribbble"> </i>
                            </a>
                        </li>
                    </ul>


                </div>



                <div class="footer-copyright text-center py-3">© 2018 Copyright:
                    <a href="https://mdbootstrap.com/bootstrap-tutorial/"> MDBootstrap.com</a>
                </div>

            </footer>
        );
    }
}

export default Footer;