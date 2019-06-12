#################
# Builder stage #
#################

FROM alpine:3.9 AS builder
LABEL maintainer="mark@flaneur.tv"
LABEL license=MIT

RUN apk update \
    && apk add --virtual build-dependencies \
        build-base \
        gcc \
        wget \
        git

COPY . /usr/src/

RUN cd /usr/src && \
  make && \
  cp client /usr/bin/prompegdec && \
  chmod +x /usr/bin/prompegdec


#################
# Runtime stage #
#################

FROM alpine:3.9 AS runtime
LABEL maintainer="mark@flaneur.tv"
LABEL license=MIT

RUN apk update \
  && apk add bash libstdc++ libgcc

# COPY prompeg.sh /usr/bin/prompeg.sh
COPY --from=builder /usr/bin/prompegdec /usr/bin/prompegdec

ENTRYPOINT /usr/bin/prompegdec
