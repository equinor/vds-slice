package vds

type InvalidArgument struct {
	message string
}

func (e *InvalidArgument) Error() string {
	return e.message
}

func NewInvalidArgument(msg string) *InvalidArgument {
	return &InvalidArgument{
		message: msg,
	}
}
